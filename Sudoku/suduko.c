#include "sudukoH.h"
int main() {
    int sudoku[SIZE][SIZE] = {0};
    if (!read_sudoku_from_file("%-sudoku.txt", sudoku)) {
        return 1;
    }

    printf("Initial Sudoku:\n");
    print_sudoku(sudoku);

    generate_cnf(sudoku, "sudoku.cnf");

    Solver s;
    init_solver(&s);
    int ret = parse(&s, "sudoku.cnf");
    if (ret != 0) {
        printf("Parse error: %d\n", ret);
        free_solver(&s);
        return 1;
    }

    ret = solve(&s);
    if (ret == 10) {
        printf("SAT\n");
        int solution[SIZE][SIZE];
        extract_solution(&s, solution);
        printf("Solution:\n");
        print_sudoku(solution);
    } else if (ret == 20) {
        printf("UNSAT\n");
    } else {
        printf("Unknown result: %d\n", ret);
    }

    free_solver(&s);
    system("pause");
    return 0;
}