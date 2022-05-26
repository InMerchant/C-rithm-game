/*
키 입력은 'A''S''L'';'
맵 바꾸는 줄은 109줄에 있음
음악은 wav파일 형식만 받을 수 있음
*/



#pragma warning(disable:4996)
#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <mmsystem.h>
#pragma comment(lib,"Winmm.lib")

#define BUFSIZE 500

#define MaxVisualPanTime 300
#define MISS 0

int PanVisualTimer = 0;

int TargetPass[5] = { 0 };
int LastPan = -1;

int Combo = 0;
int MaxCombo = 0;

//맵 로드관련
#define MAX_STR_LEN 500

#define S_GENERAL 1
#define S_EDITOR 2
#define S_METADATA 3
#define S_DIFFICULTY 4
#define S_EVENTS 5
#define S_TIMINGPOINT 6
#define S_HOBJECT 7

#define MAX_TSTAMP 300000
#define M_ROW 4
#define MAX_PSTAMP 2000

int NotePoints[MAX_TSTAMP][M_ROW] = { 0 };		// 노트 클릭 표시
int ImagePoints[MAX_TSTAMP][M_ROW] = { 0 };		// 노트 표시용
int TPoint_array_section = 0;					// 타이밍 포인트 위치
int Last_Note_pos = 0;							// 마지막 노트의 시간
int _KEY_COUNT_ = 4;							// 키 카운트 (4키)

// 게임 이미지 위치 설정
int Start_Pos = 150;
// 읽을 노트 길이 (ms)
#define READ_NOTE_MIL 580
// 노트 이미지 길이
int note_width = 0;
int note_height = 0;
// 타이머 선언
int PlayTimer = 0;
int start_time = 0;
// 맵 시작 기본값
int mapPlay = FALSE;
int map_playing = FALSE;
// 입력 받을 키 선언
#define KEY_A 0x41
#define KEY_B 0x53
#define KEY_C 0x4C
#define KEY_D 0xBA
// 키 입력 변수
int KeyDown[4] = { 0 };
int KeyLight[4] = { 0 };
// 콘솔 조작을 위한 변수
static HWND hWnd;
static HINSTANCE hInst;
//폰트관련
HFONT font;
HFONT font_combo;
RECT rt;

//맵 데이터 구조체
struct MetaData_Set {
	char Title[300];
	char TitleUnicode[300];
	char Artist[100];
	char ArtistUnicode[100];
	char Creator[100];
	char Version[100];
	char Source[200];
	char Tags[200];
	int BeatmapID;
	int BeatmapSetID;
}M_MetaData;

//맵 기본 설정값
struct General_MapSet {
	char AudioFilename[100];
	int AudioLeadIn;
	int PreviewTime;
	int Countdown;
	float StackLeniency;
}M_General;

// 스레드 선언부
HANDLE TimerHandle;
HANDLE KeyPressHandle;
HANDLE PanTimeHandle;
HANDLE GameHandle;

// 맵 종류
//char* MapClass = "Ice - Entrance (Alberiansyah) [DustMoon's NM].map";
char* MapClass = "Ice - Entrance (Alberiansyah) [HD].map";

// 맵 파일 읽을 때 사용하는 구조체
struct Difficulty_Set {
	float HPDrainRate;
	int CircleSize;
	float OverallDifficulty;
	int ApproachRate;
}M_Difficulty;
struct TimingPoint_Set {
	int time;
	double beatLength;
	int meter;
	int Volume;
	int uninherited;
	int effects;
}TimingPoints[MAX_PSTAMP];

//커서 가리기
void ClearCursor() {
	CONSOLE_CURSOR_INFO cursorInfo = { 0, };
	cursorInfo.dwSize = 1; //커서 굵기 (1 ~ 100)
	cursorInfo.bVisible = FALSE; //커서 Visible TRUE(보임) FALSE(숨김)
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// 빈칸 자르기
int Trim(char* line, char line2[]) {
	int len = 0;
	char cpTrim[MAX_STR_LEN];
	int xMan = 0;
	int i;

	len = strlen(line);				// 원본 문자열 길이
	if (len >= MAX_STR_LEN)
	{
		puts("string too long");
		return -1;
	}

	strcpy(cpTrim, line);			// 문자열 복사

	// 앞에거 잘라내기
	for (i = 0; i < len; i++)
	{
		if (cpTrim[i] == ' ' || cpTrim[i] == '\t')
			xMan++;
		else
			break;
	}

	// 뒤에거 잘라내기
	for (i = len - 1; i >= 0; i--)
	{
		if (cpTrim[i] == ' ' || cpTrim[i] == '\t' || cpTrim[i] == '\n')
			cpTrim[i] = '\0';
		else
			break;
	}

	strcpy(line2, cpTrim + xMan);		// line2에 붙여 넣기

	return strlen(line);
}

// General 읽기
void ReadProperty_General(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "AudioFilename") == 0) {  // 오디오 파일 이름
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_General.AudioFilename, nstr);
	}
	else if (strcmp(nstr, "AudioLeadIn") == 0) { // 오디오 시작 위치
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_General.AudioLeadIn = atoi(ptr);
	}
	else if (strcmp(nstr, "PreviewTime") == 0) { // 미리보기 시작 위치
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_General.PreviewTime = atoi(ptr);
	}
	else if (strcmp(nstr, "Countdown") == 0) { // 초반 카운트 다운 여부
		Trim(ptr, nstr);
		ptr = strtok(NULL, ":");
		M_General.Countdown = atoi(ptr);
	}
	else if (strcmp(nstr, "StackLeniency") == 0) { // 플레이 래이턴시
		Trim(ptr, nstr);
		ptr = strtok(NULL, ":");
		M_General.StackLeniency = atof(ptr);
	}
}

// MetaData 읽기
void ReadProperty_MetaData(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "Title") == 0) {				// 타이틀 영어
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Title, nstr);
	}
	else if (strcmp(nstr, "TitleUnicode") == 0) {	// 타이틀 원어
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.TitleUnicode, nstr);
	}
	else if (strcmp(nstr, "Artist") == 0) {			// 아티스트 영어
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Artist, nstr);
	}
	else if (strcmp(nstr, "ArtistUnicode") == 0) {	// 아티스트 원어
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.ArtistUnicode, nstr);
	}
	else if (strcmp(nstr, "Creator") == 0) {		// 제작자
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Creator, nstr);
	}
	else if (strcmp(nstr, "Version") == 0) {		// 세부 맵 이름
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Version, nstr);
	}
	else if (strcmp(nstr, "Source") == 0) {			// 맵 소스
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Source, nstr);
	}
	else if (strcmp(nstr, "Tags") == 0) {			// 맵 태그
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Tags, nstr);
	}
	else if (strcmp(nstr, "BeatmapID") == 0) {		// 비트맵 ID
		ptr = strtok(NULL, ":");
		M_MetaData.BeatmapID = atoi(ptr);
	}
	else if (strcmp(nstr, "BeatmapSetID") == 0) {	// 비트맵 셋 ID
		ptr = strtok(NULL, ":");
		M_MetaData.BeatmapSetID = atoi(ptr);
	}
}

// Difficulty 읽기
void ReadProperty_Difficulty(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "HPDrainRate") == 0) {				// HP 피통
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.HPDrainRate = atof(nstr);
	}
	else if (strcmp(nstr, "CircleSize") == 0) {			// 키 개수
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.CircleSize = atof(nstr);
	}
	else if (strcmp(nstr, "OverallDifficulty") == 0) {	// 판정 난이도1
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.OverallDifficulty = atof(nstr);
	}
	else if (strcmp(nstr, "ApproachRate") == 0) {		// 판정 난이도2
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.ApproachRate = atof(nstr);
	}
}

// TimingPoint 읽기
void ReadProperty_TimingPoint(char* str) {
	char* ptr = strtok(str, ",");
	char strs[200] = { 0, };
	int i = 0, key = 0;
	while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
	{
		Trim(ptr, strs);			  // 좌우 빈칸 제거
		switch (i)
		{
		case 0:
			TimingPoints[TPoint_array_section].time = atoi(strs);			// 변속 출현 시간
			break;
		case 2:
			TimingPoints[TPoint_array_section].meter = atoi(strs);			// 변속 길이
			break;
		case 5:
			TimingPoints[TPoint_array_section].Volume = atoi(strs);			// 볼륨 조절
			break;
		case 6:
			TimingPoints[TPoint_array_section].uninherited = atoi(strs);	// 상속 여부 (BPM 조절 or 스크롤 속도 조절)
			break;
		case 7:
			TimingPoints[TPoint_array_section].effects = atoi(strs);		// 효과
			break;
		case 1:
			TimingPoints[TPoint_array_section].beatLength = atof(strs);		// 비트 길이
			break;
		}
		ptr = strtok(NULL, ",");      // 다음 문자열을 잘라서 포인터를 반환
		i++;
	}
	TPoint_array_section++;
}

// 노트 문자열 읽고 만들기
void TPoint(char* TStr) {
	int row[6] = { 0 };
	char buff[200] = { NULL };
	Trim(TStr, buff);		// 양끝 빈 공간 지우기

	char* ptr = strtok(buff, ","); // ','를 기준으로 자르기
	char strs[200] = { 0, };
	int i = 0, key = 0;
	while (ptr != NULL)               // 자른 문자열이 나오지 않을 때까지 반복
	{
		Trim(ptr, strs);			  // 좌우 빈칸 제거
		row[i] = atoi(strs);          // 값 추가
		ptr = strtok(NULL, ",");      // 다음 문자열을 잘라서 포인터를 반환
		i++;
	}

	switch (row[0])					  // 0번째 값 (키 위치)
	{
	case 64:	// 0번째 키
		key = 0;
		break;
	case 192:	// 1번째 키
		key = 1;
		break;
	case 320:	// 2번째 키
		key = 2;
		break;
	case 448:	//3번째 키
		key = 3;
		break;
	}

	NotePoints[row[2]][key] = 1;				// 노트 추가
	Last_Note_pos = row[2];						// 마지막 노트 위치 갱신
	if (row[3] == 128) {					    // 롱노트 판정 (0: 단노트, 128: 롱노트)
		for (int n = row[2]; n <= row[5]; n++)  // 롱노트 끝날때까지 채우기 : row[5] 까지
			ImagePoints[n][key] = 2;			// 롱노트 채우기
	}
	else {
		ImagePoints[row[2]][key] = 1;			// 단노트 채우기
	}
}

// 파일 읽고 제목별 분류 (안쓰는건 정리)
void ReadLine_Check(char* str, int section) {

	switch (section)
	{
	case S_GENERAL:
		ReadProperty_General(str);		// 기본 설정
		break;
	case S_EDITOR:
		// Editor (Non-Use)
		break;
	case S_METADATA:
		ReadProperty_MetaData(str);		// 메타 데이터 (제목, 이름 등)
		break;
	case S_DIFFICULTY:
		//ReadProperty_Difficulty(str);	// 난이도 설정
		break;
	case S_EVENTS:
		// Events(ex. BG)
		break;
	case S_TIMINGPOINT:
		ReadProperty_TimingPoint(str);	// 타이밍 설정
		break;
	case S_HOBJECT:
		TPoint(str);					// 노트 설정
		break;
	}
}

// 맵 파일 불러오기
int LoadMapFile(char* beatmap) {
	FILE* pFile = NULL;

	pFile = fopen(beatmap, "r");
	if (pFile == NULL) {
		printf("파일 못읽었음\n");
		return 1;
	}
	if (pFile != NULL)
	{
		char strTemp[MAX_STR_LEN];
		int section = 0;

		while (!feof(pFile))
		{
			fgets(strTemp, sizeof(strTemp), pFile);
			if (strcmp(strTemp, "[General]\n") == 0) { // 기본설정
				section = S_GENERAL;
			}
			else if (strcmp(strTemp, "[Editor]\n") == 0) { // 에디터
				section = S_EDITOR;
			}
			else if (strcmp(strTemp, "[Metadata]\n") == 0) { // 메타 데이터
				section = S_METADATA;
			}
			else if (strcmp(strTemp, "[Difficulty]\n") == 0) { // 난이도
				section = S_DIFFICULTY;
			}
			else if (strcmp(strTemp, "[Events]\n") == 0) { // 이벤트
				section = S_EVENTS;
			}
			else if (strcmp(strTemp, "[TimingPoints]\n") == 0) { // 타이밍 포인트
				section = S_TIMINGPOINT;
			}
			else if (strcmp(strTemp, "[HitObjects]\n") == 0) { // 노트
				section = S_HOBJECT;
			}
			else {
				ReadLine_Check(strTemp, section); // 읽은 값들 정리

			}
		}

		if (M_General.PreviewTime == -1)
			M_General.PreviewTime = 0;
		fclose(pFile); // 파일 닫기
		return 1;
	}
	else {
		return 0;
	}
}

//맵 가져오기
void LoadMap(char* NoteMapClass) {
	char buf[500] = { NULL }; // 버퍼 선언
	if (LoadMapFile(NoteMapClass) == 0) {
		printf("Load Failed.");
	}
	sprintf(buf, "%s - %s (%s) [%s]", M_MetaData.Artist, M_MetaData.Title, M_MetaData.Creator, M_MetaData.Version);
	// 음악 제작자, 맵 이름, 맵 제작자, 버전 buffer에 쓰기
	SetConsoleTitle(buf); //버퍼 제목에 쓰기
}

// 글자 위치 변경
void RT_Change(RECT* rts, int a, int b, int c, int d) {
	// Render 함수에서 글자 표시시에 위치 지정하는 함수
	rts->left = a;
	rts->top = b;

	rts->right = c;
	rts->bottom = d;
}

// 준비화면 렌더링
void ReadyRender() {
	// Render 함수와 중복되는 주석은 달지 않음
	hWnd = GetConsoleWindow();
	hInst = GetModuleHandle(NULL);
	HDC hDC, hMemDC;
	static HDC hBackDC;
	HBITMAP hBackBitmap, hOldBitmap, hNewBitmap;
	BITMAP Bitmap;

	hDC = GetDC(hWnd);

	hMemDC = CreateCompatibleDC(hDC);
	hBackDC = CreateCompatibleDC(hDC);

	hBackBitmap = CreateCompatibleBitmap(hDC, 1280, 720);
	hOldBitmap = (HBITMAP)SelectObject(hBackDC, hBackBitmap);

	char buf[200];

	SetTextColor(hBackDC, RGB(255, 255, 255)); // 색 설정
	SetBkMode(hBackDC, TRANSPARENT);

	sprintf(buf, "Map: %s [%s]", M_MetaData.Title, M_MetaData.Version); // 맵 이름, 버전 출력
	RT_Change(&rt, 20, 30, 900, 90);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

	sprintf(buf, "Artist: %s", M_MetaData.Artist); // 음악 제작자 출력
	RT_Change(&rt, 20, 90, 700, 120);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);
	
	sprintf(buf, "Press [ENTER] To Start"); // 엔터누르면 시작 출력
	RT_Change(&rt, 20, 180, 700, 240);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

	BitBlt(hDC, 0, 0, 1280, 720, hBackDC, 0, 0, SRCCOPY); // 이미지 콘솔에 그리기

	DeleteObject(SelectObject(hBackDC, hBackBitmap));
	DeleteDC(hBackDC);
	DeleteDC(hMemDC);

	ReleaseDC(hWnd, hDC);
}

//준비상태
void Ready() {	
	ReadyRender(); // 준비 렌더링 표시
	while (1)
	{
		if (GetAsyncKeyState(VK_RETURN) & 0x0001) { // 엔터키를 누르면 나가기
			break;
		}
	}
}

// 키 입력 처리. k : 키 위치 (0,1,2,3)
void KeyDownProcess(int k) {
	for (int i = -140; i <= 140; i++) {
		if (PlayTimer + i < 0) continue; //배열 범위 규정을 위한 코드

		if (NotePoints[PlayTimer + i][k] == 1) //노트가 있을 경우
		{
			int abs_v = abs(i);						//오차를 구하기 위해 오차의 절대값

			if (abs_v <= 140) {
				PanVisualTimer = MaxVisualPanTime;  // 판정 표시 시간 초기화
				Combo++;							// 콤보 ++
				if (Combo >= MaxCombo) MaxCombo++;	// Combo가 MaxCombo보다 크면 MaxCombo 추가
			}										//오차 140ms 이내
			NotePoints[PlayTimer + i][k] = 0;		//노트 친것으로 표시
			ImagePoints[PlayTimer + i][k] = 0;		//Render 함수에서 해당 노트 표시 안함
			break;
		}
	}
}

// 노트 클릭 체크
void HitNote() {
	if (KeyDown[0]) {    // 현재 해당 키를 눌렀을 경우
		KeyDownProcess(0);   // 참이라면 KeyDownProcess함수 실행
		KeyDown[0] = FALSE;     //  KeyDown 변수를 거짓으로 되돌림
								//  꾹 누르는것을 방지하기 위해서
	}
	if (KeyDown[1]) { // 위와 동일함
		KeyDownProcess(1);
		KeyDown[1] = FALSE;
	}
	if (KeyDown[2]) {
		KeyDownProcess(2);
		KeyDown[2] = FALSE;
	}
	if (KeyDown[3]) {
		KeyDownProcess(3);
		KeyDown[3] = FALSE;
	}
}

// 게임 타이머 (스레드 함수)
void* M_Timer(void* a) {
	start_time = GetTickCount64() + 80; // 시작 시간
	while (mapPlay) {
		PlayTimer = GetTickCount64() - start_time; // 플레이 타임 = 현재시간 - 시작시간
	}
	return;
}

// 키 눌렀는지 체크
void* CheckKeyPress(void* a) {
	while (mapPlay)
	{
		//0x0000 : 이전에 누른 적이 없고 호출 시점에도 눌려있지 않은 상태
		//0x0001 : 이전에 누른 적이 있고 호출 시점에는 눌려있지 않은 상태
		//0x8000 : 이전에 누른 적이 없고 호출 시점에는 눌려있는 상태
		//0x8001 : 이전에 누른 적이 있고 호출 시점에도 눌려있는 상태

		if (GetAsyncKeyState(KEY_A) & 0x0001) //Key A를 눌렀을 경우
			KeyDown[0] = TRUE;
		if (GetAsyncKeyState(KEY_B) & 0x0001)
			KeyDown[1] = TRUE;
		if (GetAsyncKeyState(KEY_C) & 0x0001)
			KeyDown[2] = TRUE;
		if (GetAsyncKeyState(KEY_D) & 0x0001)
			KeyDown[3] = TRUE;
		HitNote();
	}
	return;
}

//판정 표시 시간 타이머
void* Pan_Timer(void* a) {
	int PanVisualTimer = 0;
	while (mapPlay) {
		if (PanVisualTimer > 0) // 판정 타이머가 0이상 (300ms 이내 키를 눌렀을 경우)
		{
			PanVisualTimer--; //타이머를 1 줄인다
		}
		Sleep(1); //1ms 쉬기
	}
}

// 게임 정지용
void* Game(void* a) {
	ClearCursor();
	while (mapPlay) {

		for (int i = PlayTimer - 130; i >= PlayTimer - 230; i--) // Miss 판정
		{
			if (i < 0) continue; // 배열을 위한 함수 (배열 인덱스에 -가 들어가는걸 방지)
			for (int j = 0; j < 4; j++)
			{
				if (NotePoints[i][j] == 1) // 치지 않은 노트가 있을 경우
				{
					ImagePoints[i][j] = -1; // 노트 치지 않은것으로 표시
					NotePoints[i][j] = -1; // Render에서 표시되지 않게 마크
					PanVisualTimer = MaxVisualPanTime;
					Combo = 0;
				}
			}
		}
	}
	return;
}

//플레이 상태 체크
void CheckPlayStatus() {
	if (PlayTimer >= Last_Note_pos + 5000) { // 노트가 끝났을 경우
		mapPlay = FALSE; // 게임 끝내기
	}
}

// 게임 플레이시 렌더링 함수
inline void Render() {
	hWnd = GetConsoleWindow();						// 자신의 콘솔 윈도우를 가져옴
	hInst = GetModuleHandle(NULL);					// 콘솔 핸들러 가져옴
	HDC hDC, hMemDC;								//표시할 메모리 할당
	static HDC hBackDC;								//표시전 렌더링할 함수 할당 (더블 버퍼링)
	HBITMAP hBackBitmap, hOldBitmap, hNewBitmap;	//표시할 비트맵
	BITMAP Bitmap;									//비트맵 선언

	hDC = GetDC(hWnd);

	hMemDC = CreateCompatibleDC(hDC);
	hBackDC = CreateCompatibleDC(hDC);

	hBackBitmap = CreateCompatibleBitmap(hDC, 1000, 500);		//렌더링 할 팔레트 크기 선언
	hOldBitmap = (HBITMAP)SelectObject(hBackDC, hBackBitmap);	//표시되는 비트맵

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("mania-note1.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); //노트 이미지 로드
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);

	note_width = Bitmap.bmWidth;
	note_height = Bitmap.bmHeight;

	for (int i = PlayTimer; i < PlayTimer + READ_NOTE_MIL; i++) // 화면에 그릴 노트 범위
	{
		if (PlayTimer < 0) //타이머가 작동하지 않으면 나가기
			break;
		for (int j = 0; j < 4; j++) // 각 키마다 확인
		{
			if (ImagePoints[i][j] == 1) // 위치에 노트가 있을경우
			{
				GdiTransparentBlt(hBackDC, j * Bitmap.bmWidth + Start_Pos, (i - PlayTimer - 500) * (-0.9), Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228));
				// 단노트 표시
			}
			else if (ImagePoints[i][j] == 2) {
				GdiTransparentBlt(hBackDC, j * Bitmap.bmWidth + Start_Pos, (i - PlayTimer - 500) * (-0.9), Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228));
				// 롱노트 표시
			}
		}
	}
	DeleteObject(hNewBitmap); // 노트 오브젝트 삭제

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("line.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); // 판정선 로드
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);
	GdiTransparentBlt(hBackDC, Start_Pos, 450, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // 그리기
	DeleteObject(hNewBitmap);

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("bar_line.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); // 노트 양쪽 구분선 로드
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);
	GdiTransparentBlt(hBackDC, Start_Pos, 0, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // 왼쪽 그리기
	GdiTransparentBlt(hBackDC, Start_Pos + (note_width * 4), 0, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // 오른쪽 그리기
	DeleteObject(hNewBitmap);

	char buf[100]; // 표시할 버퍼 생성
	SetTextColor(hBackDC, RGB(255, 255, 255)); // 텍스트 색상 설정
	SetBkMode(hBackDC, TRANSPARENT); // 글자 배경색 투명

	if (Combo > 0) { // 콤보가 0 이상일경우 (콤보가 0일 경우 제외)
		(HFONT)SelectObject(hBackDC, font_combo); // 폰트를 font_combo로 설정

		sprintf(buf, "%d", Combo); //콤보 buffer에 삽입
		RT_Change(&rt, 10, 110, 500, 160); // 글자 위치 변경
		DrawTextA(hBackDC, buf, -1, &rt, DT_CENTER); // 글자 드로우
	}

	BitBlt(hDC, 0, 0, 1000, 500, hBackDC, 0, 0, SRCCOPY); // 백그라운드에서 그린 이미지 콘솔에 그리기

	DeleteObject(SelectObject(hBackDC, hBackBitmap)); // 사용한 오브젝트 정리
	DeleteObject(hNewBitmap);
	DeleteDC(hBackDC);
	DeleteDC(hMemDC);

	ReleaseDC(hWnd, hDC); // 윈도우 할당 해제
}

//게임 시작 함수
void PlayMap() {
	// 폰트 지정
	font = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Aller");
	font_combo = CreateFont(48, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Slant");

	char buf[BUFSIZE] = { NULL };

	// 스레드 생성
	TimerHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)M_Timer, 0, 0, NULL); // 타이머 스레드
	KeyPressHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)CheckKeyPress, 0, 0, NULL); // 키입력용
	PanTimeHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Pan_Timer, 0, 0, NULL); // 판정용
	GameHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Game, 0, 0, NULL); // 게임 관리용

	mapPlay = TRUE; // 게임 플레이 시작
	while (mapPlay) {
		CheckPlayStatus(); // 게임 상태 체크
		Render(); // 게임 화면 렌더링 시작
	}
}

// 게임 마무리 함수
void GameClear() {
	char* buffs[200] = { NULL };
	sprintf(buffs, "MaxCombo : %d", MaxCombo);

	HFONT fnt_combo = CreateFont(36, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Cunia");
	while (1) {
		// Render 함수와 중복되는 주석은 달지 않음
		hWnd = GetConsoleWindow();
		hInst = GetModuleHandle(NULL);
		HDC hDC, hMemDC;
		static HDC hBackDC;
		HBITMAP hBackBitmap, hOldBitmap, hNewBitmap;
		BITMAP Bitmap;

		hDC = GetDC(hWnd);

		hMemDC = CreateCompatibleDC(hDC);
		hBackDC = CreateCompatibleDC(hDC);

		hBackBitmap = CreateCompatibleBitmap(hDC, 1280, 720);
		hOldBitmap = (HBITMAP)SelectObject(hBackDC, hBackBitmap);

		char buf[200];


		sprintf(buf, "Map: %s [%s]", M_MetaData.Title, M_MetaData.Version);
		RT_Change(&rt, 20, 30, 900, 90);
		DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

		(HFONT)SelectObject(hBackDC, fnt_combo);

		SetTextColor(hBackDC, RGB(255, 255, 255));
		SetBkMode(hBackDC, TRANSPARENT);

		sprintf(buf, "MaxCombo: %d\n", MaxCombo);	// 최대 콤보
		RT_Change(&rt, 75, 300, 500, 370);
		DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

		BitBlt(hDC, 0, 0, 1280, 720, hBackDC, 0, 0, SRCCOPY); // 이미지 표시

		DeleteObject(SelectObject(hBackDC, hBackBitmap));
		DeleteDC(hBackDC);
		DeleteDC(hMemDC);

		ReleaseDC(hWnd, hDC);
	}
}

int main(void) {
	//시작 작업
	ClearCursor();					//커서 가리기
	LoadMap(MapClass);	//맵 가져오기
	Ready();						//준비상태
	
	//음악 재생
	PlaySound(TEXT("Ice - Entrance.wav"), 0, SND_FILENAME | SND_ASYNC);
	PlayMap();						//게임시작

	GameClear();					//게임이 끝났을 시

	return 0;
}