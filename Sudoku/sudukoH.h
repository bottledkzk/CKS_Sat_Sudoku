#include "../Sat/CDCL.h"


#define SIZE 9
#define BOX_SIZE 3

// 读取数独初始格局从文件
int read_sudoku_from_file(const char* filename, int sudoku[SIZE][SIZE]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Could not open file %s\n", filename);
        return 0;
    }

    // 计算文件行数
    int lines = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') lines++;
    }
    rewind(file);

    // 随机选择一行
    srand(time(NULL));
    int line_index = rand() % lines;
    int current_line = 0;
    char buffer[100]; // 足够大的缓冲区

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (current_line == line_index) {
            break;
        }
        current_line++;
    }
    fclose(file);

    // 移除换行符（如果有）
    buffer[strcspn(buffer, "\n")] = '\0';

    // 检查缓冲区长度
    if (strlen(buffer) < SIZE * SIZE) {
        printf("Invalid line length: %s\n", buffer);
        return 0;
    }

    // 解析该行到sudoku数组
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char c = buffer[i * SIZE + j];
            if (c == '.') {
                sudoku[i][j] = 0;
            } else if (c >= '1' && c <= '9') {
                sudoku[i][j] = c - '0';
            } else {
                printf("Invalid character: %c\n", c);
                return 0;
            }
        }
    }

    return 1;
}

// 生成CNF文件，包括撇对角线约束和窗口约束
void generate_cnf(int sudoku[SIZE][SIZE], const char* cnf_filename) {
    FILE* cnf_file = fopen(cnf_filename, "w");
    if (!cnf_file) {
        printf("Could not create CNF file\n");
        return;
    }

    // 首先计算子句数
    int known_count = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (sudoku[i][j] != 0) {
                known_count++;
            }
        }
    }

    // 基本约束
    int total_clauses = 81 + 2916 * 4 + known_count;
    
    // 添加撇对角线约束
    total_clauses += 9 * 36; // 9个数字，每个数字有C(9,2)=36对约束
    
    // 添加窗口约束（两个窗口）
    total_clauses += 2 * 9 * 36; // 2个窗口，每个窗口9个数字，每个数字有C(9,2)=36对约束
    
    fprintf(cnf_file, "p cnf 729 %d\n", total_clauses);

    // 每个单元格至少一个数字
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 1; k <= SIZE; k++) {
                int var = i * 81 + j * 9 + k;
                fprintf(cnf_file, "%d ", var);
            }
            fprintf(cnf_file, "0\n");
        }
    }

    // 每个单元格至多一个数字
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k1 = 1; k1 <= SIZE; k1++) {
                for (int k2 = k1 + 1; k2 <= SIZE; k2++) {
                    int var1 = i * 81 + j * 9 + k1;
                    int var2 = i * 81 + j * 9 + k2;
                    fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
                }
            }
        }
    }

    // 每行每个数字至多一次
    for (int i = 0; i < SIZE; i++) {
        for (int k = 1; k <= SIZE; k++) {
            for (int j1 = 0; j1 < SIZE; j1++) {
                for (int j2 = j1 + 1; j2 < SIZE; j2++) {
                    int var1 = i * 81 + j1 * 9 + k;
                    int var2 = i * 81 + j2 * 9 + k;
                    fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
                }
            }
        }
    }

    // 每列每个数字至多一次
    for (int j = 0; j < SIZE; j++) {
        for (int k = 1; k <= SIZE; k++) {
            for (int i1 = 0; i1 < SIZE; i1++) {
                for (int i2 = i1 + 1; i2 < SIZE; i2++) {
                    int var1 = i1 * 81 + j * 9 + k;
                    int var2 = i2 * 81 + j * 9 + k;
                    fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
                }
            }
        }
    }

    // 每宫每个数字至多一次
    for (int box_i = 0; box_i < SIZE; box_i += BOX_SIZE) {
        for (int box_j = 0; box_j < SIZE; box_j += BOX_SIZE) {
            for (int k = 1; k <= SIZE; k++) {
                // 遍历宫内的所有单元格对
                for (int i1 = box_i; i1 < box_i + BOX_SIZE; i1++) {
                    for (int j1 = box_j; j1 < box_j + BOX_SIZE; j1++) {
                        for (int i2 = box_i; i2 < box_i + BOX_SIZE; i2++) {
                            for (int j2 = box_j; j2 < box_j + BOX_SIZE; j2++) {
                                if (i1 * 9 + j1 < i2 * 9 + j2) {
                                    int var1 = i1 * 81 + j1 * 9 + k;
                                    int var2 = i2 * 81 + j2 * 9 + k;
                                    fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 添加撇对角线约束（从右上到左下的对角线）
    for (int k = 1; k <= SIZE; k++) {
        for (int i1 = 0; i1 < SIZE; i1++) {
            int j1 = SIZE - 1 - i1; // 对角线上的单元格 (i, SIZE-1-i)
            for (int i2 = i1 + 1; i2 < SIZE; i2++) {
                int j2 = SIZE - 1 - i2;
                int var1 = i1 * 81 + j1 * 9 + k;
                int var2 = i2 * 81 + j2 * 9 + k;
                fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
            }
        }
    }

    // 添加窗口约束
    // 左上窗口: r2p2,r2p3,r2p4,r3p2,r3p3,r3p4,r4p2,r4p3,r4p4
    // 注意：我们的索引是从0开始的，所以对应行1-3，列1-3
    int top_left_window[9][2] = {
        {1, 1}, {1, 2}, {1, 3},
        {2, 1}, {2, 2}, {2, 3},
        {3, 1}, {3, 2}, {3, 3}
    };
    
    for (int k = 1; k <= SIZE; k++) {
        for (int i = 0; i < 9; i++) {
            for (int j = i + 1; j < 9; j++) {
                int row1 = top_left_window[i][0];
                int col1 = top_left_window[i][1];
                int row2 = top_left_window[j][0];
                int col2 = top_left_window[j][1];
                
                int var1 = row1 * 81 + col1 * 9 + k;
                int var2 = row2 * 81 + col2 * 9 + k;
                fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
            }
        }
    }
    
    // 右下窗口: r8p8,r8p7,r8p6,r7p8,r7p7,r7p6,r6p8,r6p7,r6p6
    // 注意：我们的索引是从0开始的，所以对应行5-7，列5-7
    int bottom_right_window[9][2] = {
        {5, 5}, {5, 6}, {5, 7},
        {6, 5}, {6, 6}, {6, 7},
        {7, 5}, {7, 6}, {7, 7}
    };
    
    for (int k = 1; k <= SIZE; k++) {
        for (int i = 0; i < 9; i++) {
            for (int j = i + 1; j < 9; j++) {
                int row1 = bottom_right_window[i][0];
                int col1 = bottom_right_window[i][1];
                int row2 = bottom_right_window[j][0];
                int col2 = bottom_right_window[j][1];
                
                int var1 = row1 * 81 + col1 * 9 + k;
                int var2 = row2 * 81 + col2 * 9 + k;
                fprintf(cnf_file, "-%d -%d 0\n", var1, var2);
            }
        }
    }

    // 添加已知数字的单位子句
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (sudoku[i][j] != 0) {
                int k = sudoku[i][j];
                int var = i * 81 + j * 9 + k;
                fprintf(cnf_file, "%d 0\n", var);
            }
        }
    }

    fclose(cnf_file);
}

// 从CDCL求解器的解中提取数独解
void extract_solution(Solver* solver, int sudoku[SIZE][SIZE]) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            sudoku[i][j] = 0;
        }
    }

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            for (int k = 1; k <= SIZE; k++) {
                int var = i * 81 + j * 9 + k;
                if (var <= solver->vars && solver->value[var] == 1) {
                    sudoku[i][j] = k;
                    break;
                }
            }
        }
    }
}

// 打印数独
void print_sudoku(int sudoku[SIZE][SIZE]) {
    printf("+-------+-------+-------+\n");
    for (int i = 0; i < SIZE; i++) {
        printf("| ");
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", sudoku[i][j]);
            if (j % 3 == 2) printf("| ");
        }
        printf("\n");
        if (i % 3 == 2) {
            printf("+-------+-------+-------+\n");
        }
    }
}
