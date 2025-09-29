#include "../Sat/CDCL.h"


#define SIZE 9
#define BOX_SIZE 3
void generate_cnf_with_extra(int sudoku[SIZE][SIZE], int exclude_row, int exclude_col, int exclude_value, const char* filename);
bool is_unique_solution(int sudoku[SIZE][SIZE], int row, int col, int original_value);
bool generate_full_solution(int sudoku[SIZE][SIZE]);
bool is_unique_full_solution(int sudoku[SIZE][SIZE]); 
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
    // 为每个数字在窗口内添加互斥约束，确保窗口内数字不重复
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

int generate_sudoku_puzzle(char* output) {
    srand(time(NULL));
    
    // 1. 生成终盘T
    int full_sudoku[SIZE][SIZE] = {0};
    if (!generate_full_solution(full_sudoku)) {
        return 0; // 生成终盘失败
    }
    
    // 2. 初始化当前数独为终盘
    int current_sudoku[SIZE][SIZE];
    memcpy(current_sudoku, full_sudoku, SIZE * SIZE * sizeof(int));
    
    // 3. 随机挖空数量n (46到59之间)
    int n = rand() % 14 + 46; // 46 to 59
    
    // 4. 生成随机位置列表
    int positions[81];
    for (int i = 0; i < 81; i++) positions[i] = i;
    
    // 打乱顺序
    for (int i = 0; i < 81; i++) {
        int j = rand() % 81;
        int temp = positions[i];
        positions[i] = positions[j];
        positions[j] = temp;
    }
    
    // 5. 对于前n个位置，直接挖空
    for (int i = 0; i < n; i++) {
        int pos = positions[i];
        int row = pos / 9;
        int col = pos % 9;
        current_sudoku[row][col] = 0; // 挖空
        printf("挖空(%d,%d)\n", row, col);
    }
    
    // 6. 输出当前数独为一字符串
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (current_sudoku[i][j] == 0)
                output[i * 9 + j] = '.';
            else
                output[i * 9 + j] = '0' + current_sudoku[i][j];
        }
    }
    output[81] = '\0';
    
    return 1;
}


bool generate_full_solution(int sudoku[SIZE][SIZE]) {
    // 生成空数独的CNF约束
    int empty_sudoku[SIZE][SIZE] = {0};
    generate_cnf(empty_sudoku, "empty.cnf");
    
    Solver solver;
    init_solver(&solver);
    char* EMPTY1 = (char*)"empty.cnf";
    // 解析CNF文件并求解，获取一个完整的数独终盘
    if (parse(&solver, EMPTY1) != 0) {
        printf("Parse error for empty CNF\n");
        free_solver(&solver);
        return false;
    }
    int ret = solve(&solver);
    if (ret != 10) { // 10 表示 SAT，有解
        printf("Solve error for empty CNF: %d\n", ret);
        free_solver(&solver);
        return false;
    }
    extract_solution(&solver, sudoku);
    free_solver(&solver);
    return true;
}

bool is_unique_solution(int sudoku[SIZE][SIZE], int row, int col, int original_value) {
    int temp_sudoku[SIZE][SIZE];
    memcpy(temp_sudoku, sudoku, SIZE * SIZE * sizeof(int));
    temp_sudoku[row][col] = 0;
    char *EXTRA = (char*)"temp.cnf";
    generate_cnf_with_extra(temp_sudoku, row, col, original_value, EXTRA);
    
    Solver solver;
    init_solver(&solver);
    char *TEMP = (char*)"temp.cnf";
    int ret = parse(&solver, TEMP);
    if (ret != 0) {
        printf("Parse error for temp CNF\n");
        free_solver(&solver);
        return false;
    }
    ret = solve(&solver);
    free_solver(&solver);
    
    return (ret == 20); 
}
void generate_cnf_with_extra(int sudoku[SIZE][SIZE], int exclude_row, int exclude_col, int exclude_value, const char* filename) {
    generate_cnf(sudoku, filename);
    
    FILE* f = fopen(filename, "a");
    if (!f) {
        printf("Cannot open file for append: %s\n", filename);
        return;
    }
    int var = exclude_row * 81 + exclude_col * 9 + exclude_value;
    fprintf(f, "-%d 0\n", var);
    fclose(f);
}

bool is_unique_full_solution(int sudoku[SIZE][SIZE]) {
    generate_cnf(sudoku, "full.cnf");
    
    FILE* f = fopen("full.cnf", "a");
    if (!f) {
        printf("Cannot open file for append: full.cnf\n");
        return false;
    }
    
    // 添加一个子句：至少有一个格子与终盘不同
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int var = i * 81 + j * 9 + sudoku[i][j];
            fprintf(f, "-%d ", var);
        }
    }
    fprintf(f, "0\n");
    fclose(f);
    
    Solver solver;
    init_solver(&solver);
    char *filename = (char*)"full.cnf";
    int ret = parse(&solver, filename);
    if (ret != 0) {
        printf("Parse error in is_unique_full_solution\n");
        free_solver(&solver);
        return false;
    }
    ret = solve(&solver);
    free_solver(&solver);
    
    return (ret == 20); // 20表示无解，说明唯一
}