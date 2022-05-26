/*
Ű �Է��� 'A''S''L'';'
�� �ٲٴ� ���� 109�ٿ� ����
������ wav���� ���ĸ� ���� �� ����
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

//�� �ε����
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

int NotePoints[MAX_TSTAMP][M_ROW] = { 0 };		// ��Ʈ Ŭ�� ǥ��
int ImagePoints[MAX_TSTAMP][M_ROW] = { 0 };		// ��Ʈ ǥ�ÿ�
int TPoint_array_section = 0;					// Ÿ�̹� ����Ʈ ��ġ
int Last_Note_pos = 0;							// ������ ��Ʈ�� �ð�
int _KEY_COUNT_ = 4;							// Ű ī��Ʈ (4Ű)

// ���� �̹��� ��ġ ����
int Start_Pos = 150;
// ���� ��Ʈ ���� (ms)
#define READ_NOTE_MIL 580
// ��Ʈ �̹��� ����
int note_width = 0;
int note_height = 0;
// Ÿ�̸� ����
int PlayTimer = 0;
int start_time = 0;
// �� ���� �⺻��
int mapPlay = FALSE;
int map_playing = FALSE;
// �Է� ���� Ű ����
#define KEY_A 0x41
#define KEY_B 0x53
#define KEY_C 0x4C
#define KEY_D 0xBA
// Ű �Է� ����
int KeyDown[4] = { 0 };
int KeyLight[4] = { 0 };
// �ܼ� ������ ���� ����
static HWND hWnd;
static HINSTANCE hInst;
//��Ʈ����
HFONT font;
HFONT font_combo;
RECT rt;

//�� ������ ����ü
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

//�� �⺻ ������
struct General_MapSet {
	char AudioFilename[100];
	int AudioLeadIn;
	int PreviewTime;
	int Countdown;
	float StackLeniency;
}M_General;

// ������ �����
HANDLE TimerHandle;
HANDLE KeyPressHandle;
HANDLE PanTimeHandle;
HANDLE GameHandle;

// �� ����
//char* MapClass = "Ice - Entrance (Alberiansyah) [DustMoon's NM].map";
char* MapClass = "Ice - Entrance (Alberiansyah) [HD].map";

// �� ���� ���� �� ����ϴ� ����ü
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

//Ŀ�� ������
void ClearCursor() {
	CONSOLE_CURSOR_INFO cursorInfo = { 0, };
	cursorInfo.dwSize = 1; //Ŀ�� ���� (1 ~ 100)
	cursorInfo.bVisible = FALSE; //Ŀ�� Visible TRUE(����) FALSE(����)
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}

// ��ĭ �ڸ���
int Trim(char* line, char line2[]) {
	int len = 0;
	char cpTrim[MAX_STR_LEN];
	int xMan = 0;
	int i;

	len = strlen(line);				// ���� ���ڿ� ����
	if (len >= MAX_STR_LEN)
	{
		puts("string too long");
		return -1;
	}

	strcpy(cpTrim, line);			// ���ڿ� ����

	// �տ��� �߶󳻱�
	for (i = 0; i < len; i++)
	{
		if (cpTrim[i] == ' ' || cpTrim[i] == '\t')
			xMan++;
		else
			break;
	}

	// �ڿ��� �߶󳻱�
	for (i = len - 1; i >= 0; i--)
	{
		if (cpTrim[i] == ' ' || cpTrim[i] == '\t' || cpTrim[i] == '\n')
			cpTrim[i] = '\0';
		else
			break;
	}

	strcpy(line2, cpTrim + xMan);		// line2�� �ٿ� �ֱ�

	return strlen(line);
}

// General �б�
void ReadProperty_General(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "AudioFilename") == 0) {  // ����� ���� �̸�
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_General.AudioFilename, nstr);
	}
	else if (strcmp(nstr, "AudioLeadIn") == 0) { // ����� ���� ��ġ
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_General.AudioLeadIn = atoi(ptr);
	}
	else if (strcmp(nstr, "PreviewTime") == 0) { // �̸����� ���� ��ġ
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_General.PreviewTime = atoi(ptr);
	}
	else if (strcmp(nstr, "Countdown") == 0) { // �ʹ� ī��Ʈ �ٿ� ����
		Trim(ptr, nstr);
		ptr = strtok(NULL, ":");
		M_General.Countdown = atoi(ptr);
	}
	else if (strcmp(nstr, "StackLeniency") == 0) { // �÷��� �����Ͻ�
		Trim(ptr, nstr);
		ptr = strtok(NULL, ":");
		M_General.StackLeniency = atof(ptr);
	}
}

// MetaData �б�
void ReadProperty_MetaData(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "Title") == 0) {				// Ÿ��Ʋ ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Title, nstr);
	}
	else if (strcmp(nstr, "TitleUnicode") == 0) {	// Ÿ��Ʋ ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.TitleUnicode, nstr);
	}
	else if (strcmp(nstr, "Artist") == 0) {			// ��Ƽ��Ʈ ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Artist, nstr);
	}
	else if (strcmp(nstr, "ArtistUnicode") == 0) {	// ��Ƽ��Ʈ ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.ArtistUnicode, nstr);
	}
	else if (strcmp(nstr, "Creator") == 0) {		// ������
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Creator, nstr);
	}
	else if (strcmp(nstr, "Version") == 0) {		// ���� �� �̸�
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Version, nstr);
	}
	else if (strcmp(nstr, "Source") == 0) {			// �� �ҽ�
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Source, nstr);
	}
	else if (strcmp(nstr, "Tags") == 0) {			// �� �±�
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		strcpy(M_MetaData.Tags, nstr);
	}
	else if (strcmp(nstr, "BeatmapID") == 0) {		// ��Ʈ�� ID
		ptr = strtok(NULL, ":");
		M_MetaData.BeatmapID = atoi(ptr);
	}
	else if (strcmp(nstr, "BeatmapSetID") == 0) {	// ��Ʈ�� �� ID
		ptr = strtok(NULL, ":");
		M_MetaData.BeatmapSetID = atoi(ptr);
	}
}

// Difficulty �б�
void ReadProperty_Difficulty(char* str) {
	char nstr[200] = { NULL };
	char* ptr = strtok(str, ":");
	int i = 0;
	Trim(ptr, nstr);
	if (strcmp(nstr, "HPDrainRate") == 0) {				// HP ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.HPDrainRate = atof(nstr);
	}
	else if (strcmp(nstr, "CircleSize") == 0) {			// Ű ����
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.CircleSize = atof(nstr);
	}
	else if (strcmp(nstr, "OverallDifficulty") == 0) {	// ���� ���̵�1
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.OverallDifficulty = atof(nstr);
	}
	else if (strcmp(nstr, "ApproachRate") == 0) {		// ���� ���̵�2
		ptr = strtok(NULL, ":");
		Trim(ptr, nstr);
		M_Difficulty.ApproachRate = atof(nstr);
	}
}

// TimingPoint �б�
void ReadProperty_TimingPoint(char* str) {
	char* ptr = strtok(str, ",");
	char strs[200] = { 0, };
	int i = 0, key = 0;
	while (ptr != NULL)               // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
	{
		Trim(ptr, strs);			  // �¿� ��ĭ ����
		switch (i)
		{
		case 0:
			TimingPoints[TPoint_array_section].time = atoi(strs);			// ���� ���� �ð�
			break;
		case 2:
			TimingPoints[TPoint_array_section].meter = atoi(strs);			// ���� ����
			break;
		case 5:
			TimingPoints[TPoint_array_section].Volume = atoi(strs);			// ���� ����
			break;
		case 6:
			TimingPoints[TPoint_array_section].uninherited = atoi(strs);	// ��� ���� (BPM ���� or ��ũ�� �ӵ� ����)
			break;
		case 7:
			TimingPoints[TPoint_array_section].effects = atoi(strs);		// ȿ��
			break;
		case 1:
			TimingPoints[TPoint_array_section].beatLength = atof(strs);		// ��Ʈ ����
			break;
		}
		ptr = strtok(NULL, ",");      // ���� ���ڿ��� �߶� �����͸� ��ȯ
		i++;
	}
	TPoint_array_section++;
}

// ��Ʈ ���ڿ� �а� �����
void TPoint(char* TStr) {
	int row[6] = { 0 };
	char buff[200] = { NULL };
	Trim(TStr, buff);		// �糡 �� ���� �����

	char* ptr = strtok(buff, ","); // ','�� �������� �ڸ���
	char strs[200] = { 0, };
	int i = 0, key = 0;
	while (ptr != NULL)               // �ڸ� ���ڿ��� ������ ���� ������ �ݺ�
	{
		Trim(ptr, strs);			  // �¿� ��ĭ ����
		row[i] = atoi(strs);          // �� �߰�
		ptr = strtok(NULL, ",");      // ���� ���ڿ��� �߶� �����͸� ��ȯ
		i++;
	}

	switch (row[0])					  // 0��° �� (Ű ��ġ)
	{
	case 64:	// 0��° Ű
		key = 0;
		break;
	case 192:	// 1��° Ű
		key = 1;
		break;
	case 320:	// 2��° Ű
		key = 2;
		break;
	case 448:	//3��° Ű
		key = 3;
		break;
	}

	NotePoints[row[2]][key] = 1;				// ��Ʈ �߰�
	Last_Note_pos = row[2];						// ������ ��Ʈ ��ġ ����
	if (row[3] == 128) {					    // �ճ�Ʈ ���� (0: �ܳ�Ʈ, 128: �ճ�Ʈ)
		for (int n = row[2]; n <= row[5]; n++)  // �ճ�Ʈ ���������� ä��� : row[5] ����
			ImagePoints[n][key] = 2;			// �ճ�Ʈ ä���
	}
	else {
		ImagePoints[row[2]][key] = 1;			// �ܳ�Ʈ ä���
	}
}

// ���� �а� ���� �з� (�Ⱦ��°� ����)
void ReadLine_Check(char* str, int section) {

	switch (section)
	{
	case S_GENERAL:
		ReadProperty_General(str);		// �⺻ ����
		break;
	case S_EDITOR:
		// Editor (Non-Use)
		break;
	case S_METADATA:
		ReadProperty_MetaData(str);		// ��Ÿ ������ (����, �̸� ��)
		break;
	case S_DIFFICULTY:
		//ReadProperty_Difficulty(str);	// ���̵� ����
		break;
	case S_EVENTS:
		// Events(ex. BG)
		break;
	case S_TIMINGPOINT:
		ReadProperty_TimingPoint(str);	// Ÿ�̹� ����
		break;
	case S_HOBJECT:
		TPoint(str);					// ��Ʈ ����
		break;
	}
}

// �� ���� �ҷ�����
int LoadMapFile(char* beatmap) {
	FILE* pFile = NULL;

	pFile = fopen(beatmap, "r");
	if (pFile == NULL) {
		printf("���� ���о���\n");
		return 1;
	}
	if (pFile != NULL)
	{
		char strTemp[MAX_STR_LEN];
		int section = 0;

		while (!feof(pFile))
		{
			fgets(strTemp, sizeof(strTemp), pFile);
			if (strcmp(strTemp, "[General]\n") == 0) { // �⺻����
				section = S_GENERAL;
			}
			else if (strcmp(strTemp, "[Editor]\n") == 0) { // ������
				section = S_EDITOR;
			}
			else if (strcmp(strTemp, "[Metadata]\n") == 0) { // ��Ÿ ������
				section = S_METADATA;
			}
			else if (strcmp(strTemp, "[Difficulty]\n") == 0) { // ���̵�
				section = S_DIFFICULTY;
			}
			else if (strcmp(strTemp, "[Events]\n") == 0) { // �̺�Ʈ
				section = S_EVENTS;
			}
			else if (strcmp(strTemp, "[TimingPoints]\n") == 0) { // Ÿ�̹� ����Ʈ
				section = S_TIMINGPOINT;
			}
			else if (strcmp(strTemp, "[HitObjects]\n") == 0) { // ��Ʈ
				section = S_HOBJECT;
			}
			else {
				ReadLine_Check(strTemp, section); // ���� ���� ����

			}
		}

		if (M_General.PreviewTime == -1)
			M_General.PreviewTime = 0;
		fclose(pFile); // ���� �ݱ�
		return 1;
	}
	else {
		return 0;
	}
}

//�� ��������
void LoadMap(char* NoteMapClass) {
	char buf[500] = { NULL }; // ���� ����
	if (LoadMapFile(NoteMapClass) == 0) {
		printf("Load Failed.");
	}
	sprintf(buf, "%s - %s (%s) [%s]", M_MetaData.Artist, M_MetaData.Title, M_MetaData.Creator, M_MetaData.Version);
	// ���� ������, �� �̸�, �� ������, ���� buffer�� ����
	SetConsoleTitle(buf); //���� ���� ����
}

// ���� ��ġ ����
void RT_Change(RECT* rts, int a, int b, int c, int d) {
	// Render �Լ����� ���� ǥ�ýÿ� ��ġ �����ϴ� �Լ�
	rts->left = a;
	rts->top = b;

	rts->right = c;
	rts->bottom = d;
}

// �غ�ȭ�� ������
void ReadyRender() {
	// Render �Լ��� �ߺ��Ǵ� �ּ��� ���� ����
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

	SetTextColor(hBackDC, RGB(255, 255, 255)); // �� ����
	SetBkMode(hBackDC, TRANSPARENT);

	sprintf(buf, "Map: %s [%s]", M_MetaData.Title, M_MetaData.Version); // �� �̸�, ���� ���
	RT_Change(&rt, 20, 30, 900, 90);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

	sprintf(buf, "Artist: %s", M_MetaData.Artist); // ���� ������ ���
	RT_Change(&rt, 20, 90, 700, 120);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);
	
	sprintf(buf, "Press [ENTER] To Start"); // ���ʹ����� ���� ���
	RT_Change(&rt, 20, 180, 700, 240);
	DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

	BitBlt(hDC, 0, 0, 1280, 720, hBackDC, 0, 0, SRCCOPY); // �̹��� �ֿܼ� �׸���

	DeleteObject(SelectObject(hBackDC, hBackBitmap));
	DeleteDC(hBackDC);
	DeleteDC(hMemDC);

	ReleaseDC(hWnd, hDC);
}

//�غ����
void Ready() {	
	ReadyRender(); // �غ� ������ ǥ��
	while (1)
	{
		if (GetAsyncKeyState(VK_RETURN) & 0x0001) { // ����Ű�� ������ ������
			break;
		}
	}
}

// Ű �Է� ó��. k : Ű ��ġ (0,1,2,3)
void KeyDownProcess(int k) {
	for (int i = -140; i <= 140; i++) {
		if (PlayTimer + i < 0) continue; //�迭 ���� ������ ���� �ڵ�

		if (NotePoints[PlayTimer + i][k] == 1) //��Ʈ�� ���� ���
		{
			int abs_v = abs(i);						//������ ���ϱ� ���� ������ ���밪

			if (abs_v <= 140) {
				PanVisualTimer = MaxVisualPanTime;  // ���� ǥ�� �ð� �ʱ�ȭ
				Combo++;							// �޺� ++
				if (Combo >= MaxCombo) MaxCombo++;	// Combo�� MaxCombo���� ũ�� MaxCombo �߰�
			}										//���� 140ms �̳�
			NotePoints[PlayTimer + i][k] = 0;		//��Ʈ ģ������ ǥ��
			ImagePoints[PlayTimer + i][k] = 0;		//Render �Լ����� �ش� ��Ʈ ǥ�� ����
			break;
		}
	}
}

// ��Ʈ Ŭ�� üũ
void HitNote() {
	if (KeyDown[0]) {    // ���� �ش� Ű�� ������ ���
		KeyDownProcess(0);   // ���̶�� KeyDownProcess�Լ� ����
		KeyDown[0] = FALSE;     //  KeyDown ������ �������� �ǵ���
								//  �� �����°��� �����ϱ� ���ؼ�
	}
	if (KeyDown[1]) { // ���� ������
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

// ���� Ÿ�̸� (������ �Լ�)
void* M_Timer(void* a) {
	start_time = GetTickCount64() + 80; // ���� �ð�
	while (mapPlay) {
		PlayTimer = GetTickCount64() - start_time; // �÷��� Ÿ�� = ����ð� - ���۽ð�
	}
	return;
}

// Ű �������� üũ
void* CheckKeyPress(void* a) {
	while (mapPlay)
	{
		//0x0000 : ������ ���� ���� ���� ȣ�� �������� �������� ���� ����
		//0x0001 : ������ ���� ���� �ְ� ȣ�� �������� �������� ���� ����
		//0x8000 : ������ ���� ���� ���� ȣ�� �������� �����ִ� ����
		//0x8001 : ������ ���� ���� �ְ� ȣ�� �������� �����ִ� ����

		if (GetAsyncKeyState(KEY_A) & 0x0001) //Key A�� ������ ���
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

//���� ǥ�� �ð� Ÿ�̸�
void* Pan_Timer(void* a) {
	int PanVisualTimer = 0;
	while (mapPlay) {
		if (PanVisualTimer > 0) // ���� Ÿ�̸Ӱ� 0�̻� (300ms �̳� Ű�� ������ ���)
		{
			PanVisualTimer--; //Ÿ�̸Ӹ� 1 ���δ�
		}
		Sleep(1); //1ms ����
	}
}

// ���� ������
void* Game(void* a) {
	ClearCursor();
	while (mapPlay) {

		for (int i = PlayTimer - 130; i >= PlayTimer - 230; i--) // Miss ����
		{
			if (i < 0) continue; // �迭�� ���� �Լ� (�迭 �ε����� -�� ���°� ����)
			for (int j = 0; j < 4; j++)
			{
				if (NotePoints[i][j] == 1) // ġ�� ���� ��Ʈ�� ���� ���
				{
					ImagePoints[i][j] = -1; // ��Ʈ ġ�� ���������� ǥ��
					NotePoints[i][j] = -1; // Render���� ǥ�õ��� �ʰ� ��ũ
					PanVisualTimer = MaxVisualPanTime;
					Combo = 0;
				}
			}
		}
	}
	return;
}

//�÷��� ���� üũ
void CheckPlayStatus() {
	if (PlayTimer >= Last_Note_pos + 5000) { // ��Ʈ�� ������ ���
		mapPlay = FALSE; // ���� ������
	}
}

// ���� �÷��̽� ������ �Լ�
inline void Render() {
	hWnd = GetConsoleWindow();						// �ڽ��� �ܼ� �����츦 ������
	hInst = GetModuleHandle(NULL);					// �ܼ� �ڵ鷯 ������
	HDC hDC, hMemDC;								//ǥ���� �޸� �Ҵ�
	static HDC hBackDC;								//ǥ���� �������� �Լ� �Ҵ� (���� ���۸�)
	HBITMAP hBackBitmap, hOldBitmap, hNewBitmap;	//ǥ���� ��Ʈ��
	BITMAP Bitmap;									//��Ʈ�� ����

	hDC = GetDC(hWnd);

	hMemDC = CreateCompatibleDC(hDC);
	hBackDC = CreateCompatibleDC(hDC);

	hBackBitmap = CreateCompatibleBitmap(hDC, 1000, 500);		//������ �� �ȷ�Ʈ ũ�� ����
	hOldBitmap = (HBITMAP)SelectObject(hBackDC, hBackBitmap);	//ǥ�õǴ� ��Ʈ��

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("mania-note1.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); //��Ʈ �̹��� �ε�
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);

	note_width = Bitmap.bmWidth;
	note_height = Bitmap.bmHeight;

	for (int i = PlayTimer; i < PlayTimer + READ_NOTE_MIL; i++) // ȭ�鿡 �׸� ��Ʈ ����
	{
		if (PlayTimer < 0) //Ÿ�̸Ӱ� �۵����� ������ ������
			break;
		for (int j = 0; j < 4; j++) // �� Ű���� Ȯ��
		{
			if (ImagePoints[i][j] == 1) // ��ġ�� ��Ʈ�� �������
			{
				GdiTransparentBlt(hBackDC, j * Bitmap.bmWidth + Start_Pos, (i - PlayTimer - 500) * (-0.9), Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228));
				// �ܳ�Ʈ ǥ��
			}
			else if (ImagePoints[i][j] == 2) {
				GdiTransparentBlt(hBackDC, j * Bitmap.bmWidth + Start_Pos, (i - PlayTimer - 500) * (-0.9), Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228));
				// �ճ�Ʈ ǥ��
			}
		}
	}
	DeleteObject(hNewBitmap); // ��Ʈ ������Ʈ ����

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("line.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); // ������ �ε�
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);
	GdiTransparentBlt(hBackDC, Start_Pos, 450, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // �׸���
	DeleteObject(hNewBitmap);

	hNewBitmap = (HBITMAP)LoadImage(NULL, TEXT("bar_line.bmp"),
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION); // ��Ʈ ���� ���м� �ε�
	GetObject(hNewBitmap, sizeof(BITMAP), &Bitmap);
	SelectObject(hMemDC, hNewBitmap);
	GdiTransparentBlt(hBackDC, Start_Pos, 0, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // ���� �׸���
	GdiTransparentBlt(hBackDC, Start_Pos + (note_width * 4), 0, Bitmap.bmWidth, Bitmap.bmHeight, hMemDC, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight, RGB(255, 0, 228)); // ������ �׸���
	DeleteObject(hNewBitmap);

	char buf[100]; // ǥ���� ���� ����
	SetTextColor(hBackDC, RGB(255, 255, 255)); // �ؽ�Ʈ ���� ����
	SetBkMode(hBackDC, TRANSPARENT); // ���� ���� ����

	if (Combo > 0) { // �޺��� 0 �̻��ϰ�� (�޺��� 0�� ��� ����)
		(HFONT)SelectObject(hBackDC, font_combo); // ��Ʈ�� font_combo�� ����

		sprintf(buf, "%d", Combo); //�޺� buffer�� ����
		RT_Change(&rt, 10, 110, 500, 160); // ���� ��ġ ����
		DrawTextA(hBackDC, buf, -1, &rt, DT_CENTER); // ���� ��ο�
	}

	BitBlt(hDC, 0, 0, 1000, 500, hBackDC, 0, 0, SRCCOPY); // ��׶��忡�� �׸� �̹��� �ֿܼ� �׸���

	DeleteObject(SelectObject(hBackDC, hBackBitmap)); // ����� ������Ʈ ����
	DeleteObject(hNewBitmap);
	DeleteDC(hBackDC);
	DeleteDC(hMemDC);

	ReleaseDC(hWnd, hDC); // ������ �Ҵ� ����
}

//���� ���� �Լ�
void PlayMap() {
	// ��Ʈ ����
	font = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Aller");
	font_combo = CreateFont(48, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Slant");

	char buf[BUFSIZE] = { NULL };

	// ������ ����
	TimerHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)M_Timer, 0, 0, NULL); // Ÿ�̸� ������
	KeyPressHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)CheckKeyPress, 0, 0, NULL); // Ű�Է¿�
	PanTimeHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Pan_Timer, 0, 0, NULL); // ������
	GameHandle = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)Game, 0, 0, NULL); // ���� ������

	mapPlay = TRUE; // ���� �÷��� ����
	while (mapPlay) {
		CheckPlayStatus(); // ���� ���� üũ
		Render(); // ���� ȭ�� ������ ����
	}
}

// ���� ������ �Լ�
void GameClear() {
	char* buffs[200] = { NULL };
	sprintf(buffs, "MaxCombo : %d", MaxCombo);

	HFONT fnt_combo = CreateFont(36, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Cunia");
	while (1) {
		// Render �Լ��� �ߺ��Ǵ� �ּ��� ���� ����
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

		sprintf(buf, "MaxCombo: %d\n", MaxCombo);	// �ִ� �޺�
		RT_Change(&rt, 75, 300, 500, 370);
		DrawTextA(hBackDC, buf, -1, &rt, DT_LEFT);

		BitBlt(hDC, 0, 0, 1280, 720, hBackDC, 0, 0, SRCCOPY); // �̹��� ǥ��

		DeleteObject(SelectObject(hBackDC, hBackBitmap));
		DeleteDC(hBackDC);
		DeleteDC(hMemDC);

		ReleaseDC(hWnd, hDC);
	}
}

int main(void) {
	//���� �۾�
	ClearCursor();					//Ŀ�� ������
	LoadMap(MapClass);	//�� ��������
	Ready();						//�غ����
	
	//���� ���
	PlaySound(TEXT("Ice - Entrance.wav"), 0, SND_FILENAME | SND_ASYNC);
	PlayMap();						//���ӽ���

	GameClear();					//������ ������ ��

	return 0;
}