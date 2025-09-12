#include <graphics.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../Sudoku/sudukoH.h"  // 包含数独相关功能

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

// 数独数据结构
int sudoku[SIZE][SIZE] = {0};
int solution[SIZE][SIZE] = {0};
int initial[SIZE][SIZE] = {0}; // 初始数独，用于区分固定数字和用户输入

// 按钮结构
typedef struct {
    int x, y;
    int width, height;
    const char* text;
} Button;

// 按钮定义
Button buttons[] = {
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y, BUTTON_WIDTH, BUTTON_HEIGHT, "清空输入"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT, "调取数独"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 2*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "生成数独"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 3*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "生成解"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 4*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "验证正确"},
    {BUTTON_OFFSET_X, BUTTON_OFFSET_Y + 5*(BUTTON_HEIGHT + BUTTON_SPACING), BUTTON_WIDTH, BUTTON_HEIGHT, "退出程序"}
};

// 函数声明
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

// 主函数
int main() {
    
    initgraph(850, 700); // 创建图形窗口
    setbkcolor(WHITE);   // 设置背景色为白色
    cleardevice();       // 清屏
    
    // 设置随机种子
    srand(time(NULL));
    
    // 初始绘制
    drawTitle();
    drawAuthor();
    drawBoard();
    
    for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        drawButton(buttons[i]);
    }
    
    // 消息循环
    MOUSEMSG m;
    while (true) {
        m = GetMouseMsg();
        
        // 处理鼠标消息
        if (m.uMsg == WM_LBUTTONDOWN) {
            // 检查是否点击了按钮
            for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
                if (isPointInButton(m.x, m.y, buttons[i])) {
                    // 绘制按钮按下效果
                    drawButton(buttons[i], true);
                    Sleep(100); // 短暂延迟
                    drawButton(buttons[i], false);
                    
                    // 执行按钮功能
                    switch (i) {
                        case 0: // 清空输入
                            clearUserInput();
                            break;
                        case 1: // 调取数独 (原生成数独)
                            generateSudoku();
                            break;
                        case 2: // 生成数独 (新功能)
                            generateUniqueSudoku();
                            break;
                        case 3: // 生成解
                            solveSudoku();
                            break;
                        case 4: // 验证正确
                            if (validateSudoku()) {
                                MessageBox(GetHWnd(), "恭喜！数独解答正确。", "验证结果", MB_OK);
                            } else {
                                MessageBox(GetHWnd(), "数独解答有误，请检查。", "验证结果", MB_OK);
                            }
                            break;
                        case 5: // 退出程序
                            closegraph();
                            return 0;
                    }
                    
                    // 重绘棋盘
                    drawBoard();
                    break;
                }
            }
            
            // 检查是否点击了数独格子
            int cell = getCellFromCoord(m.x, m.y);
            if (cell != -1) {
                int row = cell / 10;
                int col = cell % 10;
                
                // 只有初始为0的格子可以编辑
                if (initial[row][col] == 0) {
                    // 弹出输入对话框
                    char input[2] = {0};
                    InputBox(input, 2, "请输入数字(1-9)", "输入数字", NULL, 0, 0, false);
                    
                    if (strlen(input) > 0) {
                        int num = atoi(input);
                        if (num >= 1 && num <= 9) {
                            sudoku[row][col] = num;
                            drawBoard();
                        } else if (num == 0) {
                            sudoku[row][col] = 0; // 清除输入
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

// 绘制数独棋盘
void drawBoard() {
    setfillcolor(WHITE);
    solidrectangle(BOARD_OFFSET_X, BOARD_OFFSET_Y, 
                  BOARD_OFFSET_X + CELL_SIZE * SIZE, 
                  BOARD_OFFSET_Y + CELL_SIZE * SIZE);
    
    setlinestyle(PS_SOLID, 1);
    setcolor(BLACK);
    
    // 绘制细线
    for (int i = 0; i <= SIZE; i++) {
        line(BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y,
             BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE * SIZE);
        line(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE,
             BOARD_OFFSET_X + CELL_SIZE * SIZE, BOARD_OFFSET_Y + i * CELL_SIZE);
    }
    
    // 绘制粗线（宫格边界）
    setlinestyle(PS_SOLID, 3);
    for (int i = 0; i <= SIZE; i += BOX_SIZE) {
        line(BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y,
             BOARD_OFFSET_X + i * CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE * SIZE);
        line(BOARD_OFFSET_X, BOARD_OFFSET_Y + i * CELL_SIZE,
             BOARD_OFFSET_X + CELL_SIZE * SIZE, BOARD_OFFSET_Y + i * CELL_SIZE);
    }
    
    // 绘制特殊约束区域（窗口）
    setlinestyle(PS_SOLID, 2);
    setcolor(LIGHTGRAY);
    
    // 左上窗口
    rectangle(BOARD_OFFSET_X + CELL_SIZE, BOARD_OFFSET_Y + CELL_SIZE,
              BOARD_OFFSET_X + 4 * CELL_SIZE, BOARD_OFFSET_Y + 4 * CELL_SIZE);
    
    // 右下窗口
    rectangle(BOARD_OFFSET_X + 5 * CELL_SIZE, BOARD_OFFSET_Y + 5 * CELL_SIZE,
              BOARD_OFFSET_X + 8 * CELL_SIZE, BOARD_OFFSET_Y + 8 * CELL_SIZE);
    
    // 绘制撇对角线
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
    
    // 绘制数字
    settextstyle(30, 0, "Arial");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (sudoku[i][j] != 0) {
                char numStr[2] = {0};
                numStr[0] = '0' + sudoku[i][j];
                
                // 初始数字用粗体，用户输入用普通字体
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

// 绘制按钮
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

// 检查点是否在按钮内
bool isPointInButton(int x, int y, Button btn) {
    return (x >= btn.x && x <= btn.x + btn.width && 
            y >= btn.y && y <= btn.y + btn.height);
}

// 从坐标获取单元格索引
int getCellFromCoord(int x, int y) {
    if (x < BOARD_OFFSET_X || x >= BOARD_OFFSET_X + CELL_SIZE * SIZE ||
        y < BOARD_OFFSET_Y || y >= BOARD_OFFSET_Y + CELL_SIZE * SIZE) {
        return -1;
    }
    
    int col = (x - BOARD_OFFSET_X) / CELL_SIZE;
    int row = (y - BOARD_OFFSET_Y) / CELL_SIZE;
    
    return row * 10 + col;
}

// 清空用户输入
void clearUserInput() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (initial[i][j] == 0) {
                sudoku[i][j] = 0;
            }
        }
    }
}

// 调取数独 - 从文件中读取
void generateSudoku() {
    // 使用read_sudoku_from_file从文件中读取数独
    if (read_sudoku_from_file("%-sudoku.txt", sudoku)) {
        // 将初始数独保存到initial数组
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                initial[i][j] = sudoku[i][j];
            }
        }
    } else {
        // 如果读取失败，显示错误消息
        MessageBox(GetHWnd(), "无法读取数独文件", "错误", MB_OK);
    }
}

// 生成唯一解数独
void generateUniqueSudoku() {
    char output[82]; // 81个字符 + 结束符
    
    if (generate_sudoku_puzzle(output)) {
        // 解析生成的数独字符串
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
        MessageBox(GetHWnd(), "生成数独失败", "错误", MB_OK);
    }
}

// 求解数独 - 集成CDCL.h中的功能
void solveSudoku() {
    // 生成CNF文件
    generate_cnf(sudoku, "sudoku.cnf");
    
    // 初始化求解器
    Solver s;
    init_solver(&s);
    
    // 解析CNF文件
    char *filenameSUDOKU;
    filenameSUDOKU = (char*)"sudoku.cnf";
    int ret = parse(&s, filenameSUDOKU);
    if (ret != 0) {
        MessageBox(GetHWnd(), "解析CNF文件失败", "错误", MB_OK);
        free_solver(&s);
        return;
    }
    
    // 求解
    ret = solve(&s);
    if (ret == 10) { // SAT
        // 提取解
        extract_solution(&s, solution);
        
        // 将解复制到sudoku数组
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {
                sudoku[i][j] = solution[i][j];
            }
        }
    } else if (ret == 20) { // UNSAT
        MessageBox(GetHWnd(), "该数独无解", "提示", MB_OK);
    } else {
        MessageBox(GetHWnd(), "求解过程中出现未知错误", "错误", MB_OK);
    }
    
    // 释放求解器资源
    free_solver(&s);
}

// 验证数独是否正确 - 考虑特殊约束
bool validateSudoku() {
    // 检查基本数独规则
    for (int i = 0; i < SIZE; i++) {
        // 检查行
        bool row[10] = {false};
        for (int j = 0; j < SIZE; j++) {
            int num = sudoku[i][j];
            if (num == 0) return false; // 有空格
            if (row[num]) return false; // 重复数字
            row[num] = true;
        }
        
        // 检查列
        bool col[10] = {false};
        for (int j = 0; j < SIZE; j++) {
            int num = sudoku[j][i];
            if (col[num]) return false; // 重复数字
            col[num] = true;
        }
    }
    
    // 检查宫
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
    
    // 检查撇对角线 (r1p9, r2p8, ..., r9p1)
    bool diag[10] = {false};
    for (int i = 0; i < SIZE; i++) {
        int j = SIZE - 1 - i;
        int num = sudoku[i][j];
        if (diag[num]) return false;
        diag[num] = true;
    }
    
    // 检查左上窗口 (r2p2, r2p3, r2p4, r3p2, r3p3, r3p4, r4p2, r4p3, r4p4)
    bool topLeft[10] = {false};
    for (int i = 1; i <= 3; i++) {
        for (int j = 1; j <= 3; j++) {
            int num = sudoku[i][j];
            if (topLeft[num]) return false;
            topLeft[num] = true;
        }
    }
    
    // 检查右下窗口 (r8p8, r8p7, r8p6, r7p8, r7p7, r7p6, r6p8, r6p7, r6p6)
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

// 绘制标题
void drawTitle() {
    settextstyle(36, 0, "黑体");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    int textWidth = textwidth("百分号数独");
    int x = (800 - textWidth) / 2;
    outtextxy(x, 30, "百分号数独");
}

// 绘制作者信息
void drawAuthor() {
    settextstyle(16, 0, "宋体");
    settextcolor(BLACK);
    setbkmode(TRANSPARENT);
    
    int textWidth = textwidth("Bottledkzk");
    int x = 800 - textWidth - 20;
    int y = 650;
    outtextxy(x, y, "Bottledkzk");
}