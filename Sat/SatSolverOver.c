#include "CDCL.h"
#include "DPLL.h"
void print_DPLL_Clauses(DPLL_Cnf *cnf) {
    printf("\nDPLL公式解析:\n");
    printf("变量数量: %d\n", cnf->lnum);
    printf("子句数量: %d\n", cnf->cnum);
    
    DPLL_Clause *current_clause = cnf->headC;
    int clause_count = 1;
    
    while (current_clause != NULL) {
        printf("子句 %d: ", clause_count++);
        DPLL_Literal *current_literal = current_clause->headL;
        
        while (current_literal != NULL) {
            printf("%d ", current_literal->order);
            current_literal = current_literal->nextL;
        }
        printf("0\n"); // 子句结束标记
        current_clause = current_clause->nextC;
    }
}

// 打印CDCL公式的函数
void print_CDCL_Clauses(Solver *s) {
    printf("\nCDCL公式解析:\n");
    printf("变量数量: %d\n", s->vars);
    printf("原始子句数量: %d\n", s->origin_clauses);
    printf("总子句数量: %d\n", s->clause_DB_size);
    
    for (int i = 0; i < s->clause_DB_size; i++) {
        Clause *c = &s->clause_DB[i];
        printf("子句 %d (LBD=%d): ", i+1, c->lbd);
        
        for (int j = 0; j < c->size; j++) {
            printf("%d ", c->lits[j]);
        }
        printf("0\n"); // 子句结束标记
    }
}
void wind_cmd_support_utf8(void){
    #ifdef WIN32
        system("chcp 65001 & cls");
    #endif
}
int main(){
    wind_cmd_support_utf8();
    printf("————Made by Bottledkzk————\n");
    printf("基于DPLL的SAT求解器\n");
    char filename[12][20]={
        "../Data/1.cnf",
        "../Data/2.cnf",
        "../Data/3.cnf",
        "../Data/4unsat.cnf",
        "../Data/5.cnf",
        "../Data/6.cnf",
        "../Data/7unsat.cnf",
        "../Data/8unsat.cnf",
        "../Data/9unsat.cnf",
        "../Data/10.cnf",
        "../Data/11unsat.cnf",
        "../Data/12.cnf"
    };
    char dplloutfile[12][17]={
        "../Data/DP1.res",
        "../Data/DP2.res",
        "../Data/DP3.res",
        "../Data/DP4.res",
        "../Data/DP5.res",
        "../Data/DP6.res",
        "../Data/DP7.res",
        "../Data/DP8.res",
        "../Data/DP9.res",
        "../Data/DP10.res",
        "../Data/DP11.res",
        "../Data/DP12.res"
    };
    char cdcloutfile[12][17]={
        "../Data/CD1.res",
        "../Data/CD2.res",
        "../Data/CD3.res",    
        "../Data/CD4.res",
        "../Data/CD5.res",
        "../Data/CD6.res",
        "../Data/CD7.res",
        "../Data/CD8.res",
        "../Data/CD9.res",
        "../Data/CD10.res",
        "../Data/CD11.res",
        "../Data/CD12.res"
    };
    int fileNum;
    printf("请输入要测试的文件数量：(1-12)\n");
    scanf("%d", &fileNum);
    if(fileNum<1 || fileNum>12){
        printf("输入错误,程序退出\n");
        printf("————Made by Bottledkzk————\n");
        system("pause");
        return 0;
    }
    int *fileOrder = (int*)malloc(fileNum*sizeof(int));
    printf("输入待测文件序号:\n");
    for(int i=0;i<fileNum;i++)
        scanf("%d", &fileOrder[i]);
    int flag = 0, *backtime = (int*)malloc(sizeof(int)), result, resultcd;
    clock_t start, end;
    double timedpConsume, timecd, differ;
    int parse_res, solve_res;
    DPLL_Cnf *cnf = NULL;
    Solver s;
    printf("\n");
    printf("结果如表      DPLL                            |      优化后\n");
    printf("文件序号      结果    回溯次数    时间          |      结果        时间              优化率\n");
  //printf(" 1             1        0       000000.00ms   |       1        0000000.00ms      99.99%\n");
    for(int i = 0; i < fileNum; i++){
        if(fileOrder[i] < 1 || fileOrder[i] > 12){
            printf("输入错误,程序退出\n");
            system("pause");
            return 0;
        }
        else if(fileOrder[i] <= 6 || fileOrder[i] == 11){
            cnf = (DPLL_Cnf*)malloc(sizeof(DPLL_Cnf));
            if(!cnf){
            printf("内存分配失败,程序退出\n");
            printf("————Made by Bottledkzk————\n");
            system("pause");
            return 0;
        }
            start = clock();
            flag = DPLL_createCnf(cnf, filename[fileOrder[i] - 1]);
            if(flag == DPLL_ERROR || flag == DPLL_FALSE || flag == DPLL_OVERFLOW){
                printf("文件%d读取失败,程序退出\n", fileOrder[i]);
                printf("————Made by Bottledkzk————\n");
                system("pause");
                return 0;
            }
            DPLL_initialCnf(cnf);
            *backtime = 0;
            flag = DPLL_DpllPro(cnf, backtime);
            if(flag == DPLL_FALSE)
                result = 0;
            else if(flag == DPLL_OK)
                result = 1;
            else{
                printf("文件%d基础DPLL失败,程序退出,flag=%d\n", fileOrder[i], flag);
                printf("————Made by Bottledkzk————\n");
                system("pause");
                return 0;
            }
            end = clock();
            timedpConsume = (double)(end - start) * 1000/ CLOCKS_PER_SEC;
            flag = DPLL_generateResult(*cnf, dplloutfile[fileOrder[i] - 1], timedpConsume, result);
            if(flag == DPLL_ERROR){
                printf("文件%d结果输出失败,程序退出\n", fileOrder[i]);
                printf("————Made by Bottledkzk————\n");
                system("pause");
                return 0;
            }
            DPLL_destoryCnf(cnf);
        }
        else
            result = -1;
        if(fileOrder[i] == 8 || fileOrder[i] == 10)
            resultcd = -1;
        else{
            start = clock();
            init_solver(&s);
            parse_res = parse(&s, filename[fileOrder[i] - 1]);
            solve_res = 0;
    
            if (parse_res == 20) {
                solve_res = 20;
            } else {
                solve_res = solve(&s);
            }
            end = clock();
            timecd = (double)(end - start) * 1000/ CLOCKS_PER_SEC;
            FILE* fout = fopen(cdcloutfile[fileOrder[i] - 1], "w");
            if (!fout) {
                printf("Cannot open output file: %s\n", cdcloutfile[fileOrder[i] - 1]);
                free_solver(&s);
                return 1;
            }
    
            if (solve_res == 10) {
                fprintf(fout, "s 1\n");
                resultcd = 1;
                print_model(&s, fout);
            } else if (solve_res == 20) {
                fprintf(fout, "s 0\nv 0\n");
                resultcd = 0;
            } else {
                fprintf(fout, "s 0\nv 0\n");
                resultcd = 0;
            }
            
            fprintf(fout, "t %.2f\n", timecd);
            fclose(fout);
            free_solver(&s);
            }
        if(resultcd == -1)
            printf(" %d             -1        #       #             |       -1        #                 #\n", fileOrder[i]);
        else if(result == -1)
            printf(" %d             -1        #       #             |       %d        %.2lfms      infinitely\n", fileOrder[i], resultcd, timecd);
        else{
            if(timedpConsume == 0)
                printf(" %d             %d        %d       %.2lfms   |       %d        %.2lfms      infinitely\n", fileOrder[i], result, *backtime, timedpConsume, resultcd, timecd);
            differ = (timedpConsume - timecd) / timedpConsume * 100;
            printf(" %d             %d        %d       %.2lfms   |       %d        %.2lfms      %.2lf%%\n", fileOrder[i], result, *backtime, timedpConsume, resultcd, timecd, differ);
        }
    }
    printf("\n");
    printf("注：1代表有解,0代表无解,-1代表超时\n");
    char response;
    printf("\n是否解析公式? 若是请按Y(y): ");
    scanf(" %c", &response);
    
    if (response == 'Y' || response == 'y') {
        printf("输入要解析的文件序号(1-12): ");
        int file_to_parse;
        scanf("%d", &file_to_parse);
        
        if (file_to_parse < 1 || file_to_parse > 12) {
            printf("无效的文件序号!\n");
        } else {
            // 解析并显示DPLL结构
            DPLL_Cnf *cnf_parse = (DPLL_Cnf*)malloc(sizeof(DPLL_Cnf));
            if (DPLL_createCnf(cnf_parse, filename[file_to_parse - 1]) == DPLL_OK) {
                print_DPLL_Clauses(cnf_parse);
                DPLL_destoryCnf(cnf_parse);
            } else {
                printf("DPLL解析失败!\n");
            }
            
            // 解析并显示CDCL结构
            Solver s_parse;
            init_solver(&s_parse);
            if (parse(&s_parse, filename[file_to_parse - 1]) == 0) {
                print_CDCL_Clauses(&s_parse);
            } else {
                printf("CDCL解析失败!\n");
            }
            free_solver(&s_parse);
        }
    }
    char chtp;
    for(int i = 0; i < fileNum; i++){
        FILE *fq;
        if(fileOrder[i] <= 6 || fileOrder[i] == 11){
            fq = fopen(dplloutfile[fileOrder[i] - 1], "r");
            if(fq == NULL){
                printf("文件%d不存在,程序退出\n", fileOrder[i]);
                system("pause");
                return 0;
            }
            while ((chtp = fgetc(fq)) != EOF) {
                putchar(chtp); // 输出字符
            }
            fclose(fq);
        }
        else
            printf("DPLL文件%d未生成\n", fileOrder[i]);
        if(fileOrder[i] != 8 && fileOrder[i] != 10){
            fq = fopen(cdcloutfile[fileOrder[i] - 1], "r");
            if(fq == NULL){
                printf("文件%d不存在,程序退出\n", fileOrder[i]);
                system("pause");
                return 0;
            }
            while ((chtp = fgetc(fq)) != EOF) {
                putchar(chtp); // 输出字符
            }
            fclose(fq);
        }
        else 
            printf("CDCL文件%d未生成\n", fileOrder[i]);
    }
    free(fileOrder);
    free(backtime);
    printf("————Made by Bottledkzk————\n");
    system("pause");
    return 0;
}