#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../Sudoku/sudukoH.h"  // ����������ع���

#define SIZE 9
#define BOX_SIZE 3
#define CELL_SIZE 60
#define BOARD_OFFSET_X 50
#define BOARD_OFFSET_Y 100
#define BUTTON_OFFSET_X 650
#define BUTTON_OFFSET_Y 120
#define BUTTON_WIDTH 150
#define BUTTON_HEIGHT 40
#define BUTTON_SPACING 20

// �������ݽṹ
int sudoku[SIZE][SIZE] = {0};
int solution[SIZE][SIZE] = {0};
int initial[SIZE][SIZE] = {0}; // ��ʼ�������������̶ֹ����ֺ��û�����

// ��ť�ṹ
typedef struct {
    int x, y;
    int width, height;
    const char* text;
} Button;

// ��ť����
Button buttons[] = {
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "�������"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT, "��ȡ����"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 2*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "��������"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 3*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "���ɽ�"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 4*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "��֤��ȷ"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 5*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "�˳�����"}
};

// ��������
void drawBoard();
void drawButton(Button btn, bool pressed = false);
bool isPointInButton(int x, int y, Button btn);
int getCellFromCoord(int x, int y);
void clearUserInput();
void generateSudoku();
void generateUniqueSudoku();
void solveSudoku();
bool validateSudoku();
void drawTitle();
void drawAuthor();

// ������
int main() {
    
    initgraph(850, 700); // ����ͼ�δ���
    setbkcolor(WHITE);   // ���ñ���ɫΪ��ɫ
    cleardevice();       // ����
    
    // �����������
    srand(time(NULL));
    
    // ��ʼ����
    drawTitle();
    drawAuthor();
    drawBoard();
    
    for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        drawButton(buttons[i]);
    }
    
    // ��Ϣѭ��
    MOUSEMSG m;
    while (true) {
        m = GetMouseMsg();
        
        // ���������Ϣ
        if (m.uMsg == WM_LBUTTONDOWN) {
            // ����Ƿ����˰�ť
            for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
                if (isPointInButton(m.x, m.y, buttons[i])) {
                    // ���ư�ť����Ч��
                    drawButton(buttons[i], true);
                    Sleep(100); // �����ӳ�
                    drawButton(buttons[i], false);
                    
                    // ִ�а�ť����
                    switch (i) {
                        case 0: // �������
                            clearUserInput();
                            break;
                        case 1: // ��ȡ���� (ԭ��������)
                            generateSudoku();
                            break;
                        case 2: // �������� (�¹���)
                            generateUniqueSudoku();
                            break;
                        case 3: // ���ɽ�
                            solveSudoku();
                            break;
                        case 4: // ��֤��ȷ
                            if (validateSudoku()) {
                                MessageBox(GetHWnd(), "��ϲ�����������ȷ��", "��֤���", MB_OK);
                            } else {
                                MessageBox(GetHWnd(), "��������������顣", "��֤���", MB_OK);
                            }
                            break;
                        case 5: // �˳�����
                            closegraph();
                            return 0;
                    }
                    
                    // �ػ�����
                    drawBoard();
                    break;
                }
            }
            
            // ����Ƿ�������������
            int cell = getCellFromCoord(m.x, m.y);
            if (cell != -1) {
                int row = cell / 10;
                int col = cell % 10;
                
                // ֻ�г�ʼΪ0�ĸ��ӿ��Ա༭
                if (initial[row][col] == 0) {
                    // ��������Ի���
                    char input[2] = {0};
                    InputBox(input, 2, "����������(1-9)", "��������", NULL, 0, 0, false);
                    
                    if (strlen(input) > 0) {
                        int num = atoi(input);
                        if (num >= 1 && num <= 9) {
                            sudoku[row][col] = num;
                            drawBoard();
                        } else if (num == 0) {
                            sudoku[row][col] = 0; // �������
                            drawBoard();
                        }
                    }
                }
            }
        }
    }
    
    closegraph();
    return 0;
}

// ������������
void drawBoard() {
    setfillcolor(WHITE);
    solidrectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y, 
                  BOARD_OFFSET_X + CELL_SIZE * SIZE, 
                  BOARD_OFFSET_Y + CELL_SIZE * SIZE);
    
    setlinestyle(PS_SOLID, 1);
    setcolor(BLACK);
    
    // ����ϸ��
    for (int i = 0; i <= SIZE; i++) {
        line(BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y,
             BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE * SIZE);
        line(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE,
             BOARD_OFFSET_X + CELL_SIZE * SIZE, BOARD_OFFSET_Y + i * CELL_SIZE);
    }
    
    // ���ƴ��ߣ�����߽磩
    setlinestyle(PS_SOLID, 3);
    for (int i = 0; i <= SIZE; i += BOX_SIZE) {
        line(BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y,
             BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE * SIZE);
        line(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE,
             BOARD_OFFSET_X + CELL_SIZE * SIZE, BOARD_OFFSET_Y + i * CELL_SIZE);
    }
    
    // ��������Լ�����򣨴��ڣ�
    setlinestyle(PS_SOLID, 2);
    setcolor(LIGHTGRAY);
    
    // ���ϴ���
    rectangle(BOARD_OFFSET_X + CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE,
              BOARD_OFFSET_X + 4 * CELL_SIZE, BOARD_OFFSET_Y + 4 * CELL_SIZE);
    
    // ���´���
    rectangle(BOARD_OFFSET_X + 5 * CELL_SIZE, BOARD_OFFSET_Y + 5 * CELL_SIZE,
              BOARD_OFFSET_X + 8 * CELL_SIZE, BOARD_OFFSET_Y + 8 * CELL_SIZE);
    
    // ����Ʋ�Խ���
    setlinestyle(PS_DOT, 1);
    for (int i = 0; i < SIZE; i++) {
        int x1 = BOARD_OFFSET_X + i * CELL_SIZE;
        int y1 = BOARD_OFFSET_Y + i * CELL_SIZE;
        int x2 = BOARD_OFFSET_X + (i+1) * CELL_SIZE;
        int y2 = BOARD_OFFSET_Y + (i+1) * CELL_SIZE;
        
        if (i < SIZE-1) {
            line(x1, y1, x2, y2);
        }
    }
    
    // ��������
    settextstyle(30, 0, "Arial");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (sudoku[i][j] != 0) {
                char numStr[2] = {0};
                numStr[0] = '0' + sudoku[i][j];
                
                // ��ʼ�����ô��壬�û���������ͨ����
                if (initial[i][j] != 0) {
                    settextstyle(30, 0, "Arial");
                    settextcolor(BLACK);
                } else {
                    settextstyle(30, 0, "Arial");
                    settextcolor(BLUE);
                }
                
                int x = BOARD_OFFSET_X + j * CELL_SIZE + CELL_SIZE/2 - 10;
                int y = BOARD_OFFSET_Y + i * CELL_SIZE + CELL_SIZE/2 - 15;
                outtextxy(x, y, numStr[0]);
            }
        }
    }
}

// ���ư�ť
void drawButton(Button btn, bool pressed) {
    if (pressed) {
        setfillcolor(DARKGRAY);
        setcolor(WHITE);
    } else {
        setfillcolor(LIGHTGRAY);
        setcolor(BLACK);
    }
    
    fillrectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);
    rectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);
    
    settextstyle(20, 0, "Arial");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    int textWidth = textwidth(btn.text);
    int textHeight = textheight(btn.text);
    int x = btn.x + (btn.width - textWidth) / 2;
    int y = btn.y + (btn.height - textHeight) / 2;
    
    outtextxy(x, y, btn.text);
}

// �����Ƿ��ڰ�ť��
bool isPointInButton(int x, int y, Button btn) {
    return (x >= btn.x && x <= btn.x + btn.width && 
            y >= btn.y && y <= btn.y + btn.height);
}

// �������ȡ��Ԫ������
int getCellFromCoord(int x, int y) {
    if (x < BOARD_OFFSET_X || x >= BOARD_OFFSET_X + CELL_SIZE * SIZE ||
        y < BOARD_OFFSET_Y || y >= BOARD_OFFSET_Y + CELL_SIZE * SIZE) {
        return -1;
    }
    
    int col = (x - BOARD_OFFSET_X) / CELL_SIZE;
    int row = (y - BOARD_OFFSET_Y) / CELL_SIZE;
    
    return row * 10 + col;
}

// ����û�����
void clearUserInput() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (initial[i][j] == 0) {
                sudoku[i][j] = 0;
            }
        }
    }
}

// ��ȡ���� - ���ļ��ж�ȡ
void generateSudoku() {
    // ʹ��read_sudoku_from_file���ļ��ж�ȡ����
    if (read_sudoku_from_file("%-sudoku.txt", sudoku)) {
        // ����ʼ�������浽initial����
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                initial[i][j] = sudoku[i][j];
            }
        }
    } else {
        // �����ȡʧ�ܣ���ʾ������Ϣ
        MessageBox(GetHWnd(), "�޷���ȡ�����ļ�", "����", MB_OK);
    }
}

// ����Ψһ������
void generateUniqueSudoku() {
    char output[82]; // 81���ַ� + ������
    
    if (generate_sudoku_puzzle(output)) {
        // �������ɵ������ַ���
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                char c = output[i * SIZE + j];
                if (c == '.') {
                    sudoku[i][j] = 0;
                    initial[i][j] = 0;
                } else {
                    sudoku[i][j] = c - '0';
                    initial[i][j] = c - '0';
                }
            }
        }
        drawBoard();
    } else {
        MessageBox(GetHWnd(), "��������ʧ��", "����", MB_OK);
    }
}

// ������� - ����CDCL.h�еĹ���
void solveSudoku() {
    // ����CNF�ļ�
    generate_cnf(sudoku, "sudoku.cnf");
    
    // ��ʼ�������
    Solver s;
    init_solver(&s);
    
    // ����CNF�ļ�
    char *filenameSUDOKU;
    filenameSUDOKU = (char*)"sudoku.cnf";
    int ret = parse(&s, filenameSUDOKU);
    if (ret != 0) {
        MessageBox(GetHWnd(), "����CNF�ļ�ʧ��", "����", MB_OK);
        free_solver(&s);
        return;
    }
    
    // ���
    ret = solve(&s);
    if (ret == 10) { // SAT
        // ��ȡ��
        extract_solution(&s, solution);
        
        // ���⸴�Ƶ�sudoku����
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                sudoku[i][j] = solution[i][j];
            }
        }
    } else if (ret == 20) { // UNSAT
        MessageBox(GetHWnd(), "�������޽�", "��ʾ", MB_OK);
    } else {
        MessageBox(GetHWnd(), "�������г���δ֪����", "����", MB_OK);
    }
    
    // �ͷ��������Դ
    free_solver(&s);
}

// ��֤�����Ƿ���ȷ - ��������Լ��
bool validateSudoku() {
    // ��������������
    for (int i = 0; i < SIZE; i++) {
        // �����
        bool row[10] = {false};
        for (int j = 0; j < SIZE; j++) {
            int num = sudoku[i][j];
            if (num == 0) return false; // �пո�
            if (row[num]) return false; // �ظ�����
            row[num] = true;
        }
        
        // �����
        bool col[10] = {false};
        for (int j = 0; j < SIZE; j++) {
            int num = sudoku[j][i];
            if (col[num]) return false; // �ظ�����
            col[num] = true;
        }
    }
    
    // ��鹬
    for (int box = 0; box < SIZE; box++) {
        bool used[10] = {false};
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        
        for (int i = startRow; i < startRow + 3; i++) {
            for (int j = startCol; j < startCol + 3; j++) {
                int num = sudoku[i][j];
                if (used[num]) return false;
                used[num] = true;
            }
        }
    }
    
    // ���Ʋ�Խ��� (r1p9, r2p8, ..., r9p1)
    bool diag[10] = {false};
    for (int i = 0; i < SIZE; i++) {
        int j = SIZE - 1 - i;
        int num = sudoku[i][j];
        if (diag[num]) return false;
        diag[num] = true;
    }
    
    // ������ϴ��� (r2p2, r2p3, r2p4, r3p2, r3p3, r3p4, r4p2, r4p3, r4p4)
    bool topLeft[10] = {false};
    for (int i = 1; i <= 3; i++) {
        for (int j = 1; j <= 3; j++) {
            int num = sudoku[i][j];
            if (topLeft[num]) return false;
            topLeft[num] = true;
        }
    }
    
    // ������´��� (r8p8, r8p7, r8p6, r7p8, r7p7, r7p6, r6p8, r6p7, r6p6)
    bool bottomRight[10] = {false};
    for (int i = 5; i <= 7; i++) {
        for (int j = 5; j <= 7; j++) {
            int num = sudoku[i][j];
            if (bottomRight[num]) return false;
            bottomRight[num] = true;
        }
    }
    
    return true;
}

// ���Ʊ���
void drawTitle() {
    settextstyle(36, 0, "����");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    int textWidth = textwidth("�ٷֺ�����");
    int x = (800 - textWidth) / 2;
    outtextxy(x, 30, "�ٷֺ�����");
}

// ����������Ϣ
void drawAuthor() {
    settextstyle(16, 0, "����");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    int textWidth = textwidth("Bottledkzk");
    int x = 800 - textWidth - 20;
    int y = 650;
    outtextxy(x, y, "Bottledkzk");
}