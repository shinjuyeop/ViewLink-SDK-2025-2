#ifndef VLKVIDEOWIDGET_H
#define VLKVIDEOWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMutex>
#include <QMenu>
#include <QAction>

struct AVFrame;
class Widget;
class VLKVideoWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	virtual void Init(int width, int height);
    virtual void Repaint(const AVFrame *frame);
	virtual void EndOfFile();
	void Clear();
    void SetMainDialog(Widget* pMainWidget) { m_pMainWidget = pMainWidget; }
    VLKVideoWidget(QWidget *parent);
	~VLKVideoWidget();
	void ShowLocalRecordFlash(bool bShow);
signals:
	void RecieveFirstFrame();
	void SignalEndOfFile();
protected:
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void resizeGL(int width, int height) override;
	virtual void dragEnterEvent(QDragEnterEvent *event) override;
	virtual void dropEvent(QDropEvent * event) override;
	virtual void contextMenuEvent(QContextMenuEvent * event) override;
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void wheelEvent(QWheelEvent* event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void timerEvent(QTimerEvent *event) override;
private slots:
	void on_SlotContextMenu(QAction* action);
	void onSlotEndOfFile();
private:
	bool InitShader();
	bool InitTexture();

	void RenderYUV();
	void DrawDirectionOperateImg(QPainter& painter);
	void DrawTelemetryInfo(QPainter& painter);
	void DrawVideoStatus(QPainter& painter);
	void DrawLocalRecordFlash(QPainter& painter);
private:
	QGLShaderProgram m_ShaderProgram;
	QOpenGLBuffer m_vbo;

	GLuint m_textureUniformY;
	GLuint m_textureUniformU;
	GLuint m_textureUniformV;

	GLuint m_vertexInAttr;
	GLuint m_textureInAttr;

	GLuint m_TextureIDY;
	GLuint m_TextureIDU;
	GLuint m_TextureIDV;

	unsigned char* m_pYData;
	unsigned char* m_pUData;
	unsigned char* m_pVData;

    int m_iPixelW;
    int m_iPixelH;

	QMutex m_mutex;
	
	bool m_bIsMagnified;

	bool m_bLButtonPressed;
	QPoint m_ptBegin;
	QPoint m_ptEnd;
	QPixmap m_pixmapBeginPos;
	QPixmap m_pixmapEndPos;
	QFont m_font;
	QFont m_fontNoSignal;
    
	int m_nStopZoomTimerID;

    Widget* m_pMainWidget;

	int m_iCheckDragDistanceTimer;
	bool m_bIsRendering;

	QPixmap m_pixmapLocalRecord;
	int m_iLocalRecordingFlashTimer;
	bool m_bFlashFlag;
};

#endif // VLKVIDEOWIDGET_H
