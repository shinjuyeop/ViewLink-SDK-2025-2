#include "VLKVideoWidget.h"
#include <QDebug>
#include "ShaderSourceCode.h"
#include <QDragEnterEvent>
#include <QMimeData>
#include <QTime>
#include "widget.h"

extern "C" {
#include "libavutil/frame.h"
};

#define LOCAL_RECORD_FLASH_ELAPSE 500

static const GLfloat vertices[] = {
    // vertex coordinate
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f,
    // texture coordinate
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f
};

VLKVideoWidget::VLKVideoWidget(QWidget *parent)
	: QOpenGLWidget(parent)
    , m_textureUniformY(0), m_textureUniformU(0), m_textureUniformV(0)
    , m_vertexInAttr(0), m_textureInAttr(0)
    , m_TextureIDY(0), m_TextureIDU(0), m_TextureIDV(0)
    , m_pYData(NULL), m_pUData(NULL), m_pVData(NULL)
	, m_iPixelW(0), m_iPixelH(0)
	, m_bIsMagnified(false)
	, m_bLButtonPressed(false)
	, m_nStopZoomTimerID(0)
    , m_pMainWidget(NULL)
	, m_iCheckDragDistanceTimer(0)
	, m_bIsRendering(false)
	, m_iLocalRecordingFlashTimer(0), m_bFlashFlag(false)
{
	setAcceptDrops(true);
	connect(this, SIGNAL(SignalEndOfFile()), this, SLOT(onSlotEndOfFile()));

	if (!m_pixmapBeginPos.load(":/image/circle_empty_48x48.png"))
	{
		qCritical() << "could not load image :/image/circle_empty_48x48.png";
	}

	if (!m_pixmapEndPos.load(":/image/circle_selected_32x32.png"))
	{
		qCritical() << "could not load image :/image/circle_selected_32x32.png";
	}

	if (!m_pixmapLocalRecord.load(":/image/RecordingFlash_32x32.png"))
	{
		qCritical() << "could not load image :/image/RecordingFlash_32x32.png";
	}

	m_fontNoSignal.setFamily("Microsoft YaHei");
	m_fontNoSignal.setPointSize(24);
	m_fontNoSignal.setBold(true);
	m_fontNoSignal.setLetterSpacing(QFont::AbsoluteSpacing, 0);
}

VLKVideoWidget::~VLKVideoWidget()
{
	makeCurrent();
	m_vbo.destroy();
	glDeleteTextures(1, &m_TextureIDY);
	glDeleteTextures(1, &m_TextureIDU);
	glDeleteTextures(1, &m_TextureIDV);
	doneCurrent();

	delete m_pYData;
	delete m_pUData;
	delete m_pVData;

	if (m_nStopZoomTimerID != 0)
	{
		killTimer(m_nStopZoomTimerID);
		m_nStopZoomTimerID = 0;
	}

	if (m_iCheckDragDistanceTimer != 0)
	{
		killTimer(m_iCheckDragDistanceTimer);
		m_iCheckDragDistanceTimer = 0;
	}
}

void VLKVideoWidget::initializeGL()
{
	m_mutex.lock();

	InitShader();
	InitTexture();

	m_mutex.unlock();
}

bool VLKVideoWidget::InitShader()
{
    // intialize openGL (function of QOpenGLFunctions)
	initializeOpenGLFunctions();

    // program load shader(vertex and texture)script
    // texture
	if (!m_ShaderProgram.addShaderFromSourceCode(QGLShader::Fragment, tString))
	{
		qDebug() << "program.addShaderFromSourceCode Fragment failed!";
	}
    // vertex
	if (!m_ShaderProgram.addShaderFromSourceCode(QGLShader::Vertex, vString))
	{
		qDebug() << "program.addShaderFromSourceCode Vertex failed!";
	}

    // bind vertex coordinate
	// m_ShaderProgram.bindAttributeLocation("vertexIn", A_VER);

    // bind texture coordinate
	// m_ShaderProgram.bindAttributeLocation("textureIn", T_VER);

    // compile shader
	if (!m_ShaderProgram.link())
	{
		qDebug() << "program.link() failed!";
	}
	
	// qDebug() << "program.bind() = " << m_ShaderProgram.bind();

	m_vertexInAttr = m_ShaderProgram.attributeLocation("vertexIn");
	m_textureInAttr = m_ShaderProgram.attributeLocation("textureIn");
	m_textureUniformY = m_ShaderProgram.uniformLocation("tex_y");
	m_textureUniformU = m_ShaderProgram.uniformLocation("tex_u");
	m_textureUniformV = m_ShaderProgram.uniformLocation("tex_v");

	m_vbo.create();
	m_vbo.bind();
	m_vbo.allocate(vertices, sizeof(vertices));
	m_vbo.release();

	return true;
}

bool VLKVideoWidget::InitTexture()
{
	if (m_TextureIDY != 0)
	{
		glDeleteTextures(1, &m_TextureIDY);
		glDeleteTextures(1, &m_TextureIDU);
		glDeleteTextures(1, &m_TextureIDV);
	}

    // generate texture
	glGenTextures(1, &m_TextureIDY);
	glGenTextures(1, &m_TextureIDU);
	glGenTextures(1, &m_TextureIDV);

	// Y
	glBindTexture(GL_TEXTURE_2D, m_TextureIDY);
    // linear interpolation
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	// U
	glBindTexture(GL_TEXTURE_2D, m_TextureIDU);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	// V
	glBindTexture(GL_TEXTURE_2D, m_TextureIDV);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width / 2, height / 2, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

	glClearColor(0.0, 0.0, 0.0, 0.0);

	return 0;
}

void VLKVideoWidget::paintGL()
{
	QPainter painter;
	painter.begin(this);
	// painter.setRenderHint(QPainter::Antialiasing, true);

	/*******************************************************************
    openGL render YUV data
	********************************************************************/
	painter.beginNativePainting();
	//Clear
	// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFrontFace(GL_CW);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	m_ShaderProgram.bind();
	RenderYUV();
	m_ShaderProgram.release();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	painter.endNativePainting();


	/*****************************************************************
    QPainter overlay text or image on video
	******************************************************************/
	DrawDirectionOperateImg(painter);
	DrawLocalRecordFlash(painter);
	if (!m_bIsRendering)
	{
		DrawVideoStatus(painter);
	}

	painter.end();	
}

void VLKVideoWidget::RenderYUV()
{
	m_mutex.lock();
	if (!m_pYData)
	{
		m_mutex.unlock();
		return;
	}

	m_ShaderProgram.enableAttributeArray(m_vertexInAttr);
	m_ShaderProgram.enableAttributeArray(m_textureInAttr);
	m_vbo.bind();
	m_ShaderProgram.setAttributeBuffer(m_vertexInAttr, GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
	m_ShaderProgram.setAttributeBuffer(m_textureInAttr, GL_FLOAT, 8 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));
	m_vbo.release();

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureIDY); // bind 0 layer to Y texture
    // copy Y data to texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iPixelW, m_iPixelH, 0, GL_RED, GL_UNSIGNED_BYTE, m_pYData);
	glUniform1i(m_textureUniformY, 0);

	glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_TextureIDU); // bind 1 layer to U texture
    // copy U data to texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iPixelW / 2, m_iPixelH / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_pUData);
	glUniform1i(m_textureUniformU, 1);

	glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_TextureIDV); // bind 2 layer to V texture
    // copy V data to texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_iPixelW / 2, m_iPixelH / 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_pVData);
	glUniform1i(m_textureUniformV, 2);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // disable attribute array to make the QPainter available
	m_ShaderProgram.disableAttributeArray(m_vertexInAttr);
	m_ShaderProgram.disableAttributeArray(m_textureInAttr);

	m_mutex.unlock();
}

void VLKVideoWidget::DrawDirectionOperateImg(QPainter& painter)
{
	if (!m_bLButtonPressed || m_ptBegin == m_ptEnd)
	{
		return;
	}
	painter.save();
	if (m_ptBegin.x() != 0 && m_ptBegin.y() != 0)
	{
		painter.drawPixmap(m_ptBegin.x() - (m_pixmapBeginPos.width()>>1), 
			m_ptBegin.y() - (m_pixmapBeginPos.height() >> 1), m_pixmapBeginPos);
	}

	if (m_ptEnd.x() != 0 && m_ptEnd.y() != 0)
	{
		painter.drawPixmap(m_ptEnd.x() - (m_pixmapEndPos.width() >> 1),
			m_ptEnd.y() - (m_pixmapEndPos.height() >> 1), m_pixmapEndPos);
	}
	painter.restore();
}

void VLKVideoWidget::DrawLocalRecordFlash(QPainter& painter)
{
	if (m_iLocalRecordingFlashTimer != 0 && m_bFlashFlag)
	{
		painter.save();
		painter.drawPixmap(this->rect().width() - m_pixmapLocalRecord.width() - 10, 10, m_pixmapLocalRecord);
		painter.restore();
	}
}

void VLKVideoWidget::DrawTelemetryInfo(QPainter& painter)
{
	painter.save();

	painter.setFont(m_font);
	painter.setPen(QPen(QColor(200, 200, 200)));
	painter.drawText(20, 20, "ALT -17m		FOV 17.5");

	painter.restore();
}

void VLKVideoWidget::DrawVideoStatus(QPainter& painter)
{
	painter.save();

	painter.setFont(m_fontNoSignal);
	painter.setPen(QPen(QColor(100, 100, 100)));

	QFontMetrics fontMetrics(m_fontNoSignal);
	int iX = (rect().width() - fontMetrics.width("No signal"))>>1;
	int iY = (rect().height() - fontMetrics.height()) >> 1;
	painter.drawText(iX, iY, "No signal");

	painter.restore();
}

void VLKVideoWidget::resizeGL(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
    // m_mutex.lock();
	// qDebug() << "resizeGL " << width << " : " << height;
    // m_mutex.unlock();
}

void VLKVideoWidget::Init(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void VLKVideoWidget::Repaint(const AVFrame *frame)
{
	if (!frame)
	{
		return;
	}
	m_mutex.lock();
    // video width or height changed, reallocate texture memory
    if (frame->width != m_iPixelW || frame->height != m_iPixelH)
	{
        if (m_pYData == NULL) // receive first video frame
		{
			m_bIsRendering = true;
			emit RecieveFirstFrame();
		}

		delete m_pYData;
		delete m_pUData;
		delete m_pVData;

        // allocate texture memory
		m_pYData = new unsigned char[frame->width * frame->height]; // Y
		memset(m_pYData, 0, frame->width * frame->height);
		m_pUData = new unsigned char[frame->width * frame->height / 4]; // U
		memset(m_pUData, 0, frame->width * frame->height / 4);
		m_pVData = new unsigned char[frame->width * frame->height / 4];// V
		memset(m_pVData, 0, frame->width * frame->height / 4);

		m_iPixelW = frame->width;
		m_iPixelH = frame->height;
	}
    if (m_iPixelW == frame->linesize[0]) // no need to handle line alignment
	{
		memcpy(m_pYData, frame->data[0], m_iPixelW * m_iPixelH);
		memcpy(m_pUData, frame->data[1], m_iPixelW * m_iPixelH / 4);
		memcpy(m_pVData, frame->data[2], m_iPixelW * m_iPixelH / 4);
	}
    else // handle line alignment
	{
		for (int i = 0; i < m_iPixelH; ++i) // Y
			memcpy(m_pYData + m_iPixelW * i, frame->data[0] + frame->linesize[0] * i, m_iPixelW);
		for (int i = 0; i < m_iPixelH / 2; ++i) // U
			memcpy(m_pUData + m_iPixelW / 2 * i, frame->data[1] + frame->linesize[1] * i, m_iPixelW);
		for (int i = 0; i < m_iPixelH / 2; ++i) // V
			memcpy(m_pVData + m_iPixelW / 2 * i, frame->data[2] + frame->linesize[2] * i, m_iPixelW);
	}
	m_mutex.unlock();

	update();
}

void VLKVideoWidget::EndOfFile()
{
	emit SignalEndOfFile();
}

void VLKVideoWidget::onSlotEndOfFile()
{
	Clear();
	m_bIsRendering = false;
	ShowLocalRecordFlash(false);
}

void VLKVideoWidget::Clear()
{
	m_mutex.lock();
	if (m_pYData != NULL)
	{
		delete m_pYData;
		m_pYData = NULL;
		delete m_pUData;
		m_pUData = NULL;
		delete m_pVData;
		m_pVData = NULL;
	}
	m_iPixelW = m_iPixelH = 0;
	m_mutex.unlock();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	update();
}

void VLKVideoWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->source() == this)
		event->ignore();
	else
		event->accept();
}

void VLKVideoWidget::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);
/*	const QMimeData* data = event->mimeData();

	int iDevID = Utils::bytesToInt(data->data("DeviceID"));
	QString strName = data->data("DeviceName");
*/
}

void VLKVideoWidget::contextMenuEvent(QContextMenuEvent * event)
{
    Q_UNUSED(event);
	// m_menu->move(cursor().pos());
	// m_menu->show();
}

void VLKVideoWidget::on_SlotContextMenu(QAction* action)
{
     Q_UNUSED(action);
}

void VLKVideoWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    // check if the device has track capability
    //...

    // send track command
    VLK_TRACK_MODE_PARAM param;
    param.emTrackTempSize = VLK_TRACK_TEMPLATE_SIZE_AUTO;
    param.emTrackSensor = VLK_SENSOR_VISIBLE1;

    QRect rc = this->rect();
    int iImgPosX = ((float)event->pos().x() / (float)rc.width()) * m_iPixelW;
    int iImgPosY = ((float)event->pos().y() / (float)rc.height()) * m_iPixelH;

    VLK_TrackTargetPositionEx(&param, iImgPosX, iImgPosY, m_iPixelW, m_iPixelH);
}

void VLKVideoWidget::wheelEvent(QWheelEvent* event)
{
    if (event->delta() > 0) // wheel is moving forward
	{
        if (m_nStopZoomTimerID != 0)
        {
            killTimer(m_nStopZoomTimerID);
            m_nStopZoomTimerID = 0;
        }
        VLK_ZoomIn(4);

        m_nStopZoomTimerID = startTimer(500);
	}
    else // wheel is moving backward
	{
        if (m_nStopZoomTimerID != 0)
        {
            killTimer(m_nStopZoomTimerID);
            m_nStopZoomTimerID = 0;
        }
        VLK_ZoomOut(4);

        m_nStopZoomTimerID = startTimer(500);
	}
}

void VLKVideoWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		m_bLButtonPressed = true;
		m_ptBegin = event->pos();
		m_ptEnd = event->pos();
		m_iCheckDragDistanceTimer = startTimer(100);
		update();
	}
	else if (event->button() == Qt::RightButton)
	{
        // check if the device has track capability
        //...

        // send stop tracking command
        VLK_DisableTrackMode();
	}
}

void VLKVideoWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (m_bLButtonPressed)
	{
		m_ptEnd = event->pos();
		update();
	}
}

void VLKVideoWidget::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
	if (m_ptBegin != m_ptEnd)
	{
		if (m_iCheckDragDistanceTimer != 0)
		{
			killTimer(m_iCheckDragDistanceTimer);
			m_iCheckDragDistanceTimer = 0;
		}
		
        // send stop moving command
        VLK_Stop();
	}
	
	m_ptBegin.setX(0);
	m_ptBegin.setY(0);
	m_ptEnd.setX(0);
	m_ptEnd.setY(0);

	m_bLButtonPressed = false;
	update();
}

void VLKVideoWidget::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == m_nStopZoomTimerID)
	{
        // send stop zooming commmand
        VLK_StopZoom();

		killTimer(m_nStopZoomTimerID);
		m_nStopZoomTimerID = 0;
	}
	else if (event->timerId() == m_iCheckDragDistanceTimer)
	{
		static short sPrevHorizontalSpeed = 0;
		static short sPrevVerticalSpeed = 0;

        // calculate move speed by drag distance
		short  sHorizontalDist = m_ptEnd.x() - m_ptBegin.x();
		short sVerticalDist = m_ptBegin.y() - m_ptEnd.y();
		if (sHorizontalDist == 0 && sVerticalDist == 0)
		{
			return;
		}
		
		QRect rc = this->rect();
        short sHorizontalSpeed = ((float)sHorizontalDist / (float)rc.width()) * VLK_MAX_YAW_SPEED;
        short sVerticalSpeed = ((float)sVerticalDist / (float)rc.height()) * VLK_MAX_PITCH_SPEED;
		if (sHorizontalSpeed != sPrevHorizontalSpeed || sVerticalSpeed != sPrevVerticalSpeed)
		{
           VLK_Move(sHorizontalSpeed, sVerticalSpeed);
		}

		sPrevHorizontalSpeed = sHorizontalSpeed;
		sPrevVerticalSpeed = sVerticalSpeed;
	}
	else if (event->timerId() == m_iLocalRecordingFlashTimer)
	{
		m_bFlashFlag = !m_bFlashFlag;
	}
}

void VLKVideoWidget::ShowLocalRecordFlash(bool bShow)
{
	if (bShow)
	{
		m_bFlashFlag = true;
		m_iLocalRecordingFlashTimer = startTimer(LOCAL_RECORD_FLASH_ELAPSE);
	}
	else
	{
		m_bFlashFlag = false;
		if (m_iLocalRecordingFlashTimer !=0)
		{
			killTimer(m_iLocalRecordingFlashTimer);
			m_iLocalRecordingFlashTimer = 0;
		}
	}
}
