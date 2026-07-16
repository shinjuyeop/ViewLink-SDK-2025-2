# ViewLink SDK 빌드 및 로그 분석

조사 대상은 `demo/demo_for_qt`, `.github/workflows/build-windows.yml`, `inc/ViewLink.h`, `lib/win64`, `bin/win64` 및 저장소에 포함된 `ViewLink.dll`이다. 조사는 2026-07-16 현재 저장소 파일과 로컬 바이너리 정적 분석을 기준으로 했다.

## 선배 개발자 공유용 결론

- GitHub Actions가 만드는 `demo_for_qt.exe`는 Qt 5.15.2 + MinGW 8.1 x64의 **Release 빌드**다. 워크플로가 qmake에 `CONFIG+=release`를 명시한다.
- 데모는 `lib/win64/ViewLink.lib` import library를 통해 제조사가 미리 빌드한 `bin/win64/ViewLink.dll`을 동적 링크한다. `ViewLink_static.lib`은 현재 Qt 프로젝트에서 사용하지 않는다.
- `ViewLink.dll`은 MinGW DLL이 아니라 MSVC 14.29 계열로 만들어졌고 일반 MSVC 런타임(`MSVCP140.dll`, `VCRUNTIME140.dll`, `VCRUNTIME140_1.dll`)을 사용한다. Debug CRT 의존성은 발견되지 않았다.
- DLL 내부에 `SDK_TCP_Release` 소스 경로가 있고 일반 CRT를 사용하므로 Release 바이너리라는 강한 정황은 있다. 그러나 제조사의 빌드 설정과 PDB가 없으므로 최적화 여부 등 정확한 구성은 저장소만으로 확정할 수 없다.
- `VLKLog` 파일은 애플리케이션의 `qDebug` 로그가 아니라 **ViewLink SDK 내부 로거**가 만든다. SDK 헤더가 실행 디렉터리에 `VLKLog` 폴더를 만들라고 명시하고 DLL에도 `VLKLog.cpp`, 로그 경로 문자열이 포함돼 있다.
- SDK 내부 로거가 DLL에 포함돼 있으므로 Release SDK에서도 `VLKLog`가 발생할 수 있다. 로그가 존재한다는 사실만으로 Debug DLL이라고 판단할 수 없다.
- 이번 변경으로 애플리케이션 Release 빌드에서는 `QT_NO_DEBUG_OUTPUT`을 정의해 `qDebug`만 억제한다. `qInfo`, `qWarning`, `qCritical`, SDK의 `VLKLog`에는 영향을 주지 않는다.
- 별도 Release SDK를 바로 요청할 근거는 약하다. 먼저 제조사에 현재 DLL의 공식 빌드 구성, 로그 레벨 제어 API, 배포용 PDB 제공 가능 여부를 확인하는 것이 맞다.

## 1. 데모 애플리케이션 빌드 타입

### GitHub Actions

`.github/workflows/build-windows.yml`은 다음 환경과 명령을 사용한다.

```text
Qt: 5.15.2
Toolchain: win64_mingw81 (MinGW 8.1 x64)
qmake demo_for_qt.pro -spec win32-g++ "CONFIG+=release" "CONFIG+=ci"
mingw32-make -j2
```

따라서 CI 산출물 `demo_for_qt.exe`는 명시적인 Release 빌드다. `CONFIG+=ci`는 프로젝트 파일에서 DLL 사전 복사 단계를 건너뛰기 위한 사용자 정의 플래그이며 Debug/Release 선택과는 무관하다.

이번 변경으로 artifact 이름과 ZIP 이름도 `demo_for_qt_win64_release`로 명확히 했다.

### 로컬 qmake/Qt Creator

`demo_for_qt.pro` 자체는 `CONFIG += release`를 강제하지 않는다. 따라서 Qt Creator의 활성 빌드 구성 또는 사용자가 전달한 qmake 인자가 로컬 Debug/Release를 결정한다. Windows에서 CI와 동일하게 빌드하려면 다음 명령을 사용한다.

```text
qmake demo_for_qt.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -j2
```

현재 Linux 검증 빌드의 생성 명령에는 `-O2`와 `QT_NO_DEBUG`가 포함되어 Release 구성이 확인됐다. 이는 Windows CI 설정을 대신 증명하는 것이 아니라 로컬 검증 결과다.

`demo_for_qt.pro`에는 다음 Release 전용 설정을 추가했다.

```qmake
CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}
```

이 정의는 Qt의 `qDebug` 출력을 컴파일 시 제거한다. 오류 진단용 `qWarning`, `qCritical`과 새 Turn To 진단의 `qInfo`는 유지한다.

## 2. Qt 데모가 사용하는 SDK 파일

Windows x64 분기에서 다음과 같이 링크 및 배포한다.

- 헤더: `inc/ViewLink.h`
- 동적 링크 import library: `lib/win64/ViewLink.lib`
- 실행 시 DLL: `bin/win64/ViewLink.dll`
- 미사용 정적 라이브러리: `lib/win64/ViewLink_static.lib`, `bin/win64/ViewLink_static.lib`
- 출력 폴더: `demo/demo_for_qt/bin/win64`

qmake의 `-L../../lib/win64 -lViewLink`는 `ViewLink.lib`을 선택한다. `ViewLink_static.lib`을 지정하는 설정은 없다. CI 배포 단계는 루트의 `bin/win64/*.dll`을 패키지에 복사한다.

저장소의 다음 DLL 복사본은 SHA-256이 모두 같았다.

```text
2954c035a66fe6ca900a83d53d9b26a814eef6185ce5be17981eba97a839387d
```

- `bin/win64/ViewLink.dll`
- `demo/demo_for_windows/x64/Release/ViewLink.dll` 로컬 복사본
- `deploy/demo_for_qt_win64/ViewLink.dll` 로컬 배포 복사본

## 3. ViewLink.dll 정적 분석

### 확인된 사실

- 형식: PE32+ x86-64 Windows DLL
- PE 타임스탬프: 2025-03-27 10:32:49로 표시됨
- MSVC linker major/minor: 14.29
- `VLK_TurnTo`, `VLK_IsConnected`, `VLK_IsMotorOn`, `VLK_IsFollowMode`가 export table에 존재
- CodeView Debug Directory와 `E:\...\SDK\src\ViewLink.pdb` 경로가 바이너리에 포함됨
- 실제 `ViewLink.pdb` 또는 `ViewLink.ilk`는 저장소에 없음
- `ViewLinkd.dll`, `ViewLink_debug.dll` 같은 별도 Debug DLL은 없음
- 바이너리 문자열에 `E:\SourceCode_2022_SDK_TCP_Release\SourceCode-release\...` 경로가 반복됨

주요 DLL 의존성은 다음과 같다.

- Windows 시스템: `KERNEL32.dll`, `ADVAPI32.dll`, `SETUPAPI.dll`, `WS2_32.dll`
- MSVC C++ 런타임: `MSVCP140.dll`, `VCRUNTIME140.dll`, `VCRUNTIME140_1.dll`
- Universal CRT: `api-ms-win-crt-*.dll`

다음 의존성은 발견되지 않았다.

- Debug CRT: `MSVCP140D.dll`, `VCRUNTIME140D.dll`, `ucrtbased.dll`
- MinGW 런타임: `libgcc_s_*.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`

즉, Qt 데모 실행 파일은 MinGW로 빌드되지만 ViewLink DLL 자체는 MSVC로 빌드된 C ABI DLL이다. 배포 PC에는 ViewLink DLL이 요구하는 Microsoft Visual C++ Redistributable x64가 필요할 수 있다. `windeployqt --compiler-runtime`은 데모의 MinGW 런타임을 배포하지만 제조사 DLL의 MSVC 런타임 설치까지 보장한다고 가정하면 안 된다.

### 확정할 수 없는 부분

- DLL 컴파일 시 `/O2`, `/Od`, `_DEBUG`, `NDEBUG` 등 정확한 플래그
- 제조사가 내부적으로 부르는 공식 Debug/Release 패키지 구분
- PDB가 full/private/public 중 어떤 방식으로 생성됐는지
- 소스 경로의 `Release` 문자열과 일반 CRT 사용 외의 최종 빌드 파이프라인

CodeView/PDB 경로는 Release 빌드에도 포함될 수 있으므로 그 자체는 Debug 빌드 증거가 아니다. 반대로 일반 CRT와 `Release` 소스 경로는 Release 정황이지만 제조사의 공식 확인을 대체하지 않는다.

## 4. 로그 발생 위치

### ViewLink SDK 내부 로그

`inc/ViewLink.h` 25행과 `VLK_Init()` 설명은 애플리케이션 실행 루트에 `VLKLog` 폴더를 수동 생성하도록 요구한다. `ViewLink.dll` 문자열에도 다음이 포함된다.

- `./VLKLog/`
- `VLKLog-`
- `./%s%s%s.log`
- `Utility/VLKLog.cpp`
- 로그 레벨/모드 관련 내부 함수 문자열

실제 저장소/로컬 산출물의 `VLKLog-*.log`에는 다음과 같은 SDK 소스 파일 및 함수명이 기록돼 있다.

```text
debug:[ViewLink.cpp][99][VLK_Init]... ViewLink SDK 3.5.3 initialized ...
debug:[ViewLink.cpp][277][VLK_RegisterDevStatusCB]VLK_RegisterDevStatusCB
```

따라서 `VLKLog` 폴더의 로그는 SDK 내부에서 발생한다. 애플리케이션은 Qt 메시지 핸들러를 등록해 이 파일에 쓰지 않는다.

실행 점검에서는 `VLK_IsConnected()`와 `VLK_IsSerialPortConnected()` 같은 상태 조회도 매 호출마다 SDK debug 로그를 발생시키는 것이 관찰됐다. 따라서 새 진단 UI는 유휴 상태에서 상태 API를 계속 polling하지 않는다. 연결 이벤트와 사용자 동작 때 갱신하고, Turn To 감시 중에만 장비 상태를 최대 1초에 한 번 재확인한다. 이는 SDK 로그 정책을 변경하지 않으면서 불필요한 로그 증가를 줄인다.

공개 헤더에서 로그 레벨, 로그 모드, 로그 비활성화를 제어하는 API는 발견되지 않았다. DLL 내부 문자열만 근거로 비공개 함수를 호출하거나 DLL을 수정하면 안 된다.

### Qt 애플리케이션 로그

애플리케이션은 `qDebug`, `qInfo`, `qWarning`, `qCritical`을 사용한다. 별도 `qInstallMessageHandler`나 파일 sink가 없으므로 기본적으로 콘솔/표준 오류 또는 Windows 디버거 출력으로 전달되며 `VLKLog` 파일에는 들어가지 않는다.

- `qDebug`: 연결, 스트림 정보, 녹화 시작/종료 등 일반 진단. Release에서는 이번 `QT_NO_DEBUG_OUTPUT` 설정으로 억제.
- `qInfo`: SDK 버전과 Turn To 명령/결과. Release에서도 유지.
- `qWarning`: 녹화/SRT/메타데이터의 복구 가능한 실패.
- `qCritical`: 연결, FFmpeg 열기/디코딩/파일 기록 등 중요한 실패.

### FFmpeg 로그

`VideoObjNetwork.cpp`에는 FFmpeg 로그를 `qDebug`로 전달하는 `ffmpeg_log_callback`이 정의돼 있지만 `av_log_set_callback(ffmpeg_log_callback)` 호출은 주석 처리돼 있다. 따라서 현재 이 사용자 정의 callback은 활성화되지 않는다.

FFmpeg 자체 기본 로거는 라이브러리 내부에서 로그가 발생하면 stderr로 출력할 수 있다. 별도로 애플리케이션이 FFmpeg 로그 레벨을 설정하거나 파일로 저장하는 코드는 없다. `VideoObjNetwork.cpp`의 명시적인 FFmpeg 오류 메시지는 `qCritical`로 출력된다.

## 5. Release에서도 로그가 발생하는가

가능하다. 로그 발생 여부와 바이너리 빌드 타입은 별개의 문제다.

- ViewLink SDK: DLL 내부 로거가 Release로 보이는 바이너리에도 포함돼 있으므로 `VLKLog` 생성 가능
- Qt `qDebug`: `QT_NO_DEBUG_OUTPUT`을 정의하지 않으면 Release에서도 호출이 남을 수 있음. 이번 변경에서는 Release만 억제
- Qt `qInfo`/`qWarning`/`qCritical`: Release에서도 의도적으로 유지
- FFmpeg 기본 로그: Qt Debug/Release와 무관하게 FFmpeg 설정에 따라 출력 가능

현재 로그를 전부 막지 않은 이유는 장비 통신 및 녹화 오류 진단을 보존하기 위해서다. SDK 로그는 공식 제어 API가 확인될 때만 정책을 바꾸는 것이 안전하다.

## 6. 제조사에 문의할 내용

1. 제공된 SDK 3.5.3 `ViewLink.dll`의 공식 빌드 구성은 Release인가?
2. DLL의 정확한 MSVC toolset, 런타임 배포 요구사항 및 지원 Windows 버전은 무엇인가?
3. 운영 배포용 public PDB 또는 crash 분석용 symbol server를 제공할 수 있는가?
4. `VLKLog`의 레벨, 파일 크기, 보관 일수, 비활성화를 제어하는 공개 API 또는 설정 파일이 있는가?
5. 로그 폴더가 없을 때의 동작과 디스크 쓰기 실패가 SDK 기능에 미치는 영향은 무엇인가?
6. `VLK_TurnTo`의 yaw/pitch 기준 좌표계, 양/음 방향, Follow mode 및 외부 컨트롤러 명령과의 우선순위는 무엇인가?
7. 목표 도달 이벤트가 별도로 있는가, 또는 텔레메트리 비교가 공식 권장 방식인가?
8. `VLK_DEV_TELEMETRY::dYaw/dPitch` 갱신 주기와 명령 후 예상 지연은 얼마인가?

## 7. VLK_TurnTo 장비 테스트 기능

Qt 데모에 기존 `VLK_Move`, Home, Zoom, EO/IR 스트리밍·녹화 코드는 유지하고 별도 `VLK_TurnTo test` 그룹을 추가했다.

- 입력: yaw `-180.00..180.00`, pitch `-90.00..90.00`
- 사전 확인: 연결, motor ON, 범위 재검증
- Motor ON/OFF는 별도 명시적 버튼이며 Turn To가 자동으로 motor를 켜지 않음
- telemetry 판정은 200 ms 간격으로 수행하고, 연결/motor/Follow 상태 API는 SDK 로그 증가를 줄이기 위해 연결 이벤트·사용자 동작 및 명령 수행 중 최대 1초 간격으로 갱신. Follow mode는 변경하지 않음
- 허용 오차: yaw/pitch 각각 1.0도
- yaw 오차는 `-180/180` wrap-around를 고려
- 목표 범위에 들어온 뒤 1초간 유지되면 `Target reached`
- 전체 timeout: 10초
- 명령 전/후 telemetry와 요청값을 UI 및 `qInfo`에 기록
- yaw만 이동, pitch만 이동, 둘 다 이동, 둘 다 미이동을 timeout 결과에서 구분
- 목표에 접근한 뒤 다시 멀어지면 외부 컨트롤러 override 가능성을 표시
- telemetry 없이 물리적으로 움직이는 경우(G), telemetry는 변하지만 물리 움직임이 이상한 경우(F)는 소프트웨어만으로 확정할 수 없으므로 UI에 육안 확인 안내를 표시

### 장비 테스트 주의사항

- 물리 컨트롤러의 pitch/yaw 입력을 중립에 놓는다.
- Follow mode 상태를 확인한다. 테스트 도구가 Follow mode를 자동 변경하지 않는다.
- TCP 또는 Serial 연결 완료 표시 후 테스트한다.
- Motor가 ON인지 확인한다.
- ViewLink 프로그램과 데모가 같은 장비에 동시에 TCP 연결하지 않도록 한다.
- 짐벌 주변 충돌물을 제거하고 한 번에 큰 동작을 자동 반복하지 않는다.
- 프리셋 버튼은 입력값만 채우며 실제 명령은 `Turn To` 버튼을 눌러야 전송된다.

## 8. 검증 범위

- Linux Qt 5 빌드에서 전체 `demo_for_qt` 컴파일 및 링크 성공
- `.ui`에서 생성된 코드와 새 signal/slot 컴파일 성공
- 기존 `VideoObjNetwork`, 녹화, SRT 코드 함께 컴파일 성공
- Windows workflow가 기존 Release qmake 인자와 필수 파일 검사를 유지함을 정적 확인
- 실제 Windows MinGW CI 실행과 실제 짐벌 동작 검증은 별도 실행 필요
