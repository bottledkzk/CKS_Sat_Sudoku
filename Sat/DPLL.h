#include "SatHead.h"

#define DPLL_OK 1//成功
#define DPLL_FALSE 0//失败
#define DPLL_ERROR -1//错误
#define DPLL_OVERFLOW -2//溢出
#define DPLL_TRUE 2//满足解
#define DPLL_WRONG -3//错误解
#define DPLL_NOSOL -4//无解
#define DPLL_ABS(x) ((x) > 0 ? (x) : (-(x)))

//数据结构部分
typedef struct DPLL_LITERAL{
    int order;
    int fromClause;
    struct DPLL_LITERAL *nextL;
}DPLL_Literal;//子句中文字结构
typedef struct DPLL_CLAUSE{
    int order;
    int size;
    DPLL_Literal *headL;
    struct DPLL_CLAUSE *nextC;
}DPLL_Clause;//子句结构
typedef struct DPLL_ACTION{
    int decsionPoint;//决策点，即决策文字(赋值，决策)
    DPLL_Literal *headDL;
    DPLL_Clause *headDC;
    struct DPLL_ACTION *lastA;//指向栈的上一个操作
}DPLL_Action;//操作记录结构，主要用于还原CNf
typedef struct DPLL_CNF{
    int *literals;//完全文字数组
    int lnum;//完全文字数
    int cnum;//真实子句数
    DPLL_Clause *headC;
    int *labsnum;//各文字的真实绝对数量
    int *lyesnum;//正文字数量
    float *lrelweight;//各文字的相对权重
    DPLL_Action *topA;//栈顶操作
}DPLL_Cnf;

// 文字操作部分：
int DPLL_destoryLiteral(DPLL_Literal *literal){
    if(literal == NULL)
        return DPLL_OK;
    free(literal);
    return DPLL_OK;
}
int DPLL_addLiteral(int literal, DPLL_Clause *clause){//添加文字到子句
    DPLL_Literal *newL = (DPLL_Literal*)malloc(sizeof(DPLL_Literal));
    if(!newL)
        return DPLL_OVERFLOW;
    newL->order = literal;
    newL->fromClause = clause->order;
    newL->nextL = clause->headL;
    clause->headL= newL;
    clause->size++;
    return DPLL_OK;
}
int DPLL_isAtClause(int literal, DPLL_Clause clause){//判断文字是否在子句中
    DPLL_Literal *p = clause.headL;
    while(p){
        if(p->order == literal)
            return DPLL_OK;
        p = p->nextL;
    }
    return DPLL_FALSE;
}
int DPLL_removeLiteral(int literal, DPLL_Clause *clause, DPLL_Action *action){//从子句中删除文字并丢入操作序列
    DPLL_Literal *p = clause->headL;
    DPLL_Literal *q = NULL;
    if(action == NULL){
        if(p->order == literal){
            clause->headL = p->nextL;
            clause->size--;
            DPLL_destoryLiteral(p);
            return DPLL_OK;
        }
        while(p->nextL){
            q = p->nextL;
            if(q->order == literal){
                p->nextL = q->nextL;
                DPLL_destoryLiteral(q);
                clause->size--;
                return DPLL_OK;
            }
            p = p->nextL;
        }
    }
    if(p->order == literal){
        clause->headL = p->nextL;
        p->nextL = action->headDL;
        action->headDL = p;
        clause->size--;
        return DPLL_OK;
    }
    while(p->nextL)
    {
        q = p->nextL;
        if(q->order == literal){
            p->nextL = q->nextL;
            q->nextL = action->headDL;
            action->headDL = q;
            clause->size--;
            return DPLL_OK;
        }
        p = p->nextL;
    }
    return DPLL_FALSE;
}
int DPLL_countLiteral(DPLL_Cnf *cnf){//统计文字的真实绝对数量和相对权重
    DPLL_Clause *pC = cnf->headC;
    DPLL_Literal *pL = NULL;
    for(int i = 1; i <= cnf->lnum; i++){
        cnf->labsnum[i] = 0;
        cnf->lrelweight[i] = 0.0;
        cnf->lyesnum[i] = 0;
    }
    while(pC){
        if(pC->size == 0)
            return DPLL_ERROR;
        pL = pC->headL;
        while(pL){
            if(pL->order > 0){
                cnf->lrelweight[pL->order] += 10.0 / pC->size;
                cnf->lyesnum[DPLL_ABS(pL->order)]++;
            }
            else if(pL->order < 0)
                cnf->lrelweight[DPLL_ABS(pL->order)]-= 10.0 / pC->size;
            else if(pL->order == 0)
                return DPLL_ERROR;
            cnf->labsnum[DPLL_ABS(pL->order)]++;
            pL = pL->nextL;
        }
        pC = pC->nextC;
    }
    return DPLL_OK;
}
//子句操作部分：
int DPLL_destoryClause(DPLL_Clause *clause){
    if(clause == NULL)
        return DPLL_OK;
    DPLL_Literal *pL = clause->headL, *qL = NULL;
    while(pL){
        qL = pL->nextL;
        DPLL_destoryLiteral(pL);
        pL = qL;
    }
    free(clause);
    return DPLL_OK;
}
int DPLL_addClause(DPLL_Clause *clause, DPLL_Cnf *cnf){//添加子句到CNF
    if(clause->size == 0 || cnf->headC == NULL || clause->order == cnf->headC->order || clause->order < 0)
        return DPLL_ERROR;
    if(clause->order == 0 || clause->order < cnf->headC->order){
        clause->nextC = cnf->headC;
        cnf->headC = clause;
        cnf->cnum++;
        return DPLL_OK;
    }
    DPLL_Clause *pC = cnf->headC;
    DPLL_Clause *qC = pC->nextC;
    while(pC){
        if(qC == NULL || clause->order < qC->order){
            clause->nextC = qC;
            pC->nextC = clause;
            return DPLL_OK;
        }
        else if(clause->order == qC->order)
            return DPLL_ERROR;
        pC = qC;
        qC = pC->nextC;
    }
    return DPLL_FALSE;
}
int DPLL_emptyClause(DPLL_Clause clause){//判断子句是否为空
    if(clause.size == 0 || clause.headL == NULL)
        return DPLL_OK;
    else
        return DPLL_FALSE;
}
int DPLL_evaClause(DPLL_Clause clause,DPLL_Cnf cnf){//判断子句是否为真
    if(DPLL_emptyClause(clause))
        return DPLL_ERROR;
    DPLL_Literal *pL = clause.headL;
    while(pL){
        if(pL->order == 0)
            return DPLL_ERROR;
        if(pL->order > 0 && cnf.literals[pL->order] == 1)
            return DPLL_OK;
        else if(pL->order < 0 && cnf.literals[DPLL_ABS(pL->order)] == 0)
            return DPLL_OK;
        pL = pL->nextL;
    }
    return DPLL_FALSE;
}
int DPLL_removeClause(int order, DPLL_Cnf *cnf){//从CNF中删除子句,丢入操作序列
    if(cnf->headC == NULL || cnf->cnum == 0)
        return DPLL_ERROR;
    DPLL_Clause *tpC = NULL;
    DPLL_Clause *pC = cnf->headC;
    if(cnf->topA == NULL){
        if(order == cnf->headC->order){
        tpC = cnf->headC;
        cnf->headC = tpC->nextC;
        DPLL_destoryClause(tpC);
        cnf->cnum--;
        return DPLL_OK;
    }
    while(pC->nextC)
    {
        if(pC->nextC->order == order){
            tpC = pC->nextC;
            pC->nextC = tpC->nextC;
            DPLL_destoryClause(tpC);
            cnf->cnum--;
            return DPLL_OK;
        }
        else if(pC->nextC->order > order){
            printf("不存在该子句！\n");
            return DPLL_ERROR;
        }
        pC = pC->nextC;
    }
    }
    if(order == cnf->headC->order){
        tpC = cnf->headC;
        cnf->headC = tpC->nextC;
        tpC->nextC = cnf->topA->headDC;
        cnf->topA->headDC = tpC;
        cnf->cnum--;
        return DPLL_OK;
    }
    while(pC->nextC)
    {
        if(pC->nextC->order == order){
            tpC = pC->nextC;
            pC->nextC = tpC->nextC;
            tpC->nextC = cnf->topA->headDC;
            cnf->topA->headDC = tpC;
            cnf->cnum--;
            return DPLL_OK;
        }
        else if(pC->nextC->order > order){
            printf("不存在该子句！\n");
            return DPLL_ERROR;
        }
        pC = pC->nextC;
    }
    return DPLL_FALSE;
}

//Cnf操作部分：
int DPLL_destoryAction(DPLL_Action *action){
    if(action == NULL)
        return DPLL_OK;
    DPLL_Literal *pL = action->headDL, *qL = NULL;
    DPLL_Clause *pC = action->headDC, *qC = NULL;
    while(pL){
        qL = pL->nextL;
        DPLL_destoryLiteral(pL);
        pL = qL;
    }
    while(pC){
        qC = pC->nextC;
        DPLL_destoryClause(pC);
        pC = qC;
    }
    free(action);
    return DPLL_OK;
}
int DPLL_destoryCnf(DPLL_Cnf *cnf){
    if(cnf == NULL)
        return DPLL_OK;
    free(cnf->literals);
    free(cnf->labsnum);
    free(cnf->lrelweight);
    DPLL_Clause *pC = cnf->headC, *qC = NULL;
    DPLL_Action *pA = cnf->topA, *qA = NULL;
    while(pC){
        qC = pC->nextC;
        DPLL_destoryClause(pC);
        pC = qC;
    }
    while(pA){
        qA = pA->lastA;
        DPLL_destoryAction(pA);
        pA = qA;
    }
    free(cnf);
    return DPLL_OK;
}
int DPLL_createCnf(DPLL_Cnf *cnf, char *filename){//创建CNF
    FILE *fp = fopen(filename, "r");
    if(!fp)
        return DPLL_ERROR;
    //首先跳过 c 开头的行数，直到出现 p 开头的行数
    char ch;
    while((ch = fgetc(fp)) != EOF){
        if(ch == 'c')
            while((ch = fgetc(fp)) != EOF && ch != '\n');//跳过注释行
        else if(ch == EOF)
            return DPLL_ERROR;
        else if(ch == 'p')
            break;
    }
    //确定文件范式为cnf
    char paradigm[10];
    fgetc(fp);//跳过空格
    fscanf(fp, "%s", paradigm);
    if(strcmp(paradigm, "cnf") != 0)
        return DPLL_ERROR;
    //读取文字和子句数并初始化
    fgetc(fp);//跳过空格
    fscanf(fp, "%d %d", &cnf->lnum, &cnf->cnum);
    fgetc(fp);//跳过换行
    if(cnf->lnum <= 0 || cnf->cnum < 0)
        return DPLL_ERROR;
    cnf->literals = (int*)malloc(sizeof(int)*(cnf->lnum+1));
    if(!cnf->literals)
        return DPLL_OVERFLOW;
    cnf->labsnum = (int*)malloc(sizeof(int)*(cnf->lnum+1));
    if(!cnf->labsnum)
        return DPLL_OVERFLOW;
    cnf->lrelweight = (float*)malloc(sizeof(float)*(cnf->lnum+1));
    if(!cnf->lrelweight)
        return DPLL_OVERFLOW;
    cnf->lyesnum = (int*)malloc(sizeof(int)*(cnf->lnum+1));
    for(int i = 0; i <= cnf->lnum; i++){
        cnf->literals[i] = 0;
        cnf->labsnum[i] = 0;
        cnf->lrelweight[i] = 0;
        cnf->lyesnum[i] = 0;
    }
    cnf->headC = NULL;
    cnf->topA = NULL;
    // 读取子句和文字
    int literal, clauseSize,returnflag,trueCnum;
    clauseSize = 0, trueCnum = 0, returnflag = 1;
    DPLL_Clause *newC = NULL, *pC = NULL;
    while((fscanf(fp, "%d", &literal)) != EOF && trueCnum <= cnf->cnum)
    {
        if(literal == 0){
            if(clauseSize == 0)
                return DPLL_ERROR;
            //这里将新子句添加到CNF，根据大小插入
            if(cnf->headC == NULL)
                cnf->headC = newC;
            else{
                pC = cnf->headC;
                if(newC->size <= pC->size){
                    newC->nextC = pC;
                    cnf->headC = newC;
                }
                else{
                    while(pC){
                        if(pC->nextC == NULL || newC->size <= pC->nextC->size){
                            newC->nextC = pC->nextC;
                            pC->nextC = newC;
                            break;
                        }
                        pC = pC->nextC;
                    }
                }
            }
            trueCnum++;
            clauseSize = 0;
            newC = NULL;
            fgetc(fp);//跳过换行符
            continue;
        }
        if(clauseSize == 0){
            newC = (DPLL_Clause*)malloc(sizeof(DPLL_Clause));
            if(!newC)
                return DPLL_OVERFLOW;
            newC->order = -1;
            newC->size = 0;
            newC->headL = NULL;
            newC->nextC = NULL;
        }
        returnflag = DPLL_addLiteral(literal, newC);
        if(returnflag == DPLL_OVERFLOW)
            return DPLL_OVERFLOW;
        clauseSize++;
        fgetc(fp);//跳过空格
    }
    if(trueCnum != cnf->cnum)
        return DPLL_ERROR;
    pC = cnf->headC;
    DPLL_Literal *pL = NULL;
    int i = 0;
    while(pC){
        if(pC->size == 0)
            return DPLL_ERROR;
        pL = pC->headL;
        while(pL){
            if(pL->fromClause != -1)
                return DPLL_ERROR;
            else
                pL->fromClause = i;
            pL = pL->nextL;
        }
        pC->order = i;
        i++;
        pC = pC->nextC;
    }
    //赋予子句序号
    fclose(fp);
    returnflag = DPLL_countLiteral(cnf);
    if(returnflag == DPLL_ERROR)
        return DPLL_ERROR;
    return DPLL_OK;
}

int DPLL_emptyCnf(DPLL_Cnf cnf){//判断CNF是否为空
    if(cnf.headC == NULL && cnf.cnum == 0)
        return DPLL_OK;
    else if(cnf.headC == NULL && cnf.cnum != 0)
        return DPLL_ERROR;
    else if(cnf.headC != NULL && cnf.cnum == 0)
        return DPLL_ERROR;
    else
        return DPLL_FALSE;
}
int DPLL_initialCnf(DPLL_Cnf *cnf){//初始化CNF
    if(cnf == NULL)
        return DPLL_ERROR;
    for(int i = 1; i <= cnf->lnum; i++){
        if(cnf->lrelweight[i] >= 0)
            cnf->literals[i] = 1;
        else
            cnf->literals[i] = 0;
    }
    return DPLL_OK;
}
int DPLL_initialCnfPro(DPLL_Cnf *cnf){
    if(cnf == NULL)
        return DPLL_ERROR;
    for(int i = 1; i <= cnf->lnum; i++){
        if(cnf->lrelweight[i] > 0)
            cnf->literals[i] = 1;
        else if(cnf->lrelweight[i] < 0)
            cnf->literals[i] = 0;
    }
    return DPLL_OK;
}
int DPLL_valueCnf(DPLL_Cnf cnf){
    DPLL_Clause *pC = cnf.headC;
    if(pC == NULL)
        return DPLL_TRUE;
    int flag = 1;
    while(pC){
        flag = DPLL_evaClause(*pC, cnf);
        if(flag == DPLL_FALSE)
            return DPLL_FALSE;
        pC = pC->nextC;
    }
    return DPLL_TRUE;
}
//操作序列操作部分：
int DPLL_nodeAction(DPLL_Cnf *cnf, int decsionPoint){//进行一次决策或赋值时，生成操作序列栈顶节点,desionPoint正负代表赋值
    DPLL_Action *newA = (DPLL_Action*)malloc(sizeof(DPLL_Action));
    if(!newA)
        return DPLL_OVERFLOW;
    newA->decsionPoint = decsionPoint;
    newA->headDL = NULL;
    newA->headDC = NULL;
    newA->lastA = cnf->topA;
    cnf->topA = newA;
    return DPLL_OK;
}
int DPLL_backtrack(DPLL_Cnf *cnf){//更改当前节点赋值并回溯到上一个节点
    if(cnf->topA == NULL)
        return DPLL_FALSE;//栈为空说明无解
    //更改当前节点赋值
    if(cnf->topA->decsionPoint > 0)
        cnf->literals[cnf->topA->decsionPoint] = 0;
    else if(cnf->topA->decsionPoint < 0)
        cnf->literals[DPLL_ABS(cnf->topA->decsionPoint)] = 1;
    else
        return DPLL_ERROR;
    //先回溯子句
    DPLL_Clause *pC = cnf->topA->headDC;
    DPLL_Clause *qC = cnf->headC;
    DPLL_Clause *tC = NULL;
    while(pC){
        if(pC->order < 0)
            return DPLL_ERROR;
        qC = cnf->headC;
        if(qC == NULL || pC->order < qC->order){
            cnf->topA->headDC = pC->nextC;
            pC->nextC = qC;
            cnf->headC = pC;
            cnf->cnum++;
            pC = cnf->topA->headDC;
            continue;
        }
        else if(pC->order == qC->order)
            return DPLL_ERROR;
        while(qC){
            tC = qC->nextC;
            if(tC == NULL || pC->order < tC->order){
                cnf->topA->headDC = pC->nextC;
                pC->nextC = tC;
                qC->nextC = pC;
                break;
            }
            else if(pC->order == tC->order)
                return DPLL_ERROR;
            qC = tC;
        }
        cnf->cnum++;
        pC = cnf->topA->headDC;
    }
    //回溯文字
    DPLL_Literal *pL = cnf->topA->headDL;
    pC = cnf->headC;
    if(pC == NULL)
        return DPLL_OK;
    while(pL){
        pC = cnf->headC;
        if(pL->fromClause < 0)
            return DPLL_ERROR;
        while(pC){
            if(pC->order == pL->fromClause){
                cnf->topA->headDL = pL->nextL;
                pL->nextL = pC->headL;
                pC->headL = pL;
                pC->size++;
                break;
            }
            pC = pC->nextC;
        }
        if(pC == NULL)
            return DPLL_ERROR;
        pL = cnf->topA->headDL;
    }
    //回溯操作序列栈
    if(cnf->topA->headDC != NULL || cnf->topA->headDL != NULL){
        printf("回溯不完全\n");
        return DPLL_ERROR;
    }
    DPLL_Action *tA = cnf->topA;
    cnf->topA = cnf->topA->lastA;
    DPLL_destoryAction(tA);
    return DPLL_OK;
}

int DPLL_unitClauseDelete(DPLL_Cnf *cnf){//搜索最前面的单子句，进行单次单子句传播，返回FALSE表示无单子句，返回OK，表示进行了传播
    DPLL_Clause *pC = cnf->headC, *qC = NULL;
    if(pC == NULL)
        return DPLL_OK;
    int order = -1, literal = 0, reverse = 0, returnflag = 0;
    while(pC){
        if(pC->size == 0){
            printf("子句为零\n");
            return DPLL_ERROR;
        }
        else if(pC->size == 1){
            literal = pC->headL->order;
            if(literal > 0)
                cnf->literals[DPLL_ABS(literal)] = 1;
            else
                cnf->literals[DPLL_ABS(literal)] = 0;
            reverse = 0 - literal;
            pC = cnf->headC;
            //Test printf("含有文字的子句删除中\n", literal);
            while(pC){//删除所有含义literal的子句
                returnflag = DPLL_isAtClause(literal, *pC);
                if(returnflag == DPLL_OK){
                    order = pC->order;
                    pC = pC->nextC;
                    returnflag = DPLL_removeClause(order, cnf);
                    if(returnflag == DPLL_ERROR){
                        printf("删除子句错误\n");
                        return DPLL_ERROR;
                    }
                    continue;
                }
                pC = pC->nextC;
            }
            //Test printf("删除逆中\n");
            pC = cnf->headC;
            while(pC){//删除剩余子句中的逆
                returnflag = DPLL_isAtClause(reverse, *pC);
                if(returnflag == DPLL_OK)
                    DPLL_removeLiteral(reverse, pC, cnf->topA);
                pC = pC->nextC;
            }
            //Test printf("单子句传播成功喵\n");
            return DPLL_OK;
        }
        pC = pC->nextC;
    }
    return DPLL_FALSE;
}

int DPLL_assignWithWeight(DPLL_Cnf *cnf){//权重赋值操作，加权
    DPLL_countLiteral(cnf);
    DPLL_Clause *pC = cnf->headC;
    if(pC == NULL || cnf->lnum == 0)
        return DPLL_ERROR;
    int literal = 0, returnflag = 0, reverse = 0, order = -1;
    DPLL_initialCnfPro(cnf);
    returnflag = DPLL_valueCnf(*cnf);
    if(returnflag == DPLL_TRUE)
        return DPLL_TRUE;
    DPLL_Clause *qL = NULL;
    if(pC == NULL || pC->headL == NULL || cnf->lnum == 0)
        return DPLL_ERROR;
    literal = pC->headL->order;
    if(cnf->lrelweight[DPLL_ABS(literal)] >= 0){
        cnf->literals[DPLL_ABS(literal)] = 1;
        literal = DPLL_ABS(literal);
    }
    else{
        cnf->literals[DPLL_ABS(literal)] = 0;
        literal = 0 - DPLL_ABS(literal);
    }
    reverse = 0 - literal;
    returnflag = DPLL_nodeAction(cnf, literal);
    if(returnflag == DPLL_OVERFLOW)
        return DPLL_OVERFLOW;
    else if(returnflag == DPLL_ERROR){
        printf("操作节点创建错误\n");
        return DPLL_ERROR;
    }
    while(pC){
        if(pC->size == 0 || pC->size == 1){
            printf("赋值发现错误或者单子句\n");
            return DPLL_ERROR;
        }
        returnflag = DPLL_isAtClause(literal, *pC);
        if(returnflag == DPLL_OK){
            order = pC->order;
            pC = pC->nextC;
            returnflag = DPLL_removeClause(order, cnf);
            if(returnflag == DPLL_ERROR){
                printf("删除子句失败\n");
                return DPLL_ERROR;
            }
            continue;
        }
        returnflag = DPLL_isAtClause(reverse, *pC);
        if(returnflag == DPLL_OK){
            returnflag = DPLL_removeLiteral(reverse, pC, cnf->topA);
            if(returnflag == DPLL_ERROR){
                printf("删除逆文字失败\n");
                return DPLL_ERROR;
            }
        }
        pC = pC->nextC;
    }
    return DPLL_OK;
}

int DPLL_wrongClause(DPLL_Cnf cnf){//检查是否存在空子句
    DPLL_Clause *pC = cnf.headC;
    if(cnf.cnum == 0)
        return DPLL_FALSE;
    if(pC == NULL)
        return DPLL_ERROR;
    while(pC){
        if(pC->size == 0)
            return DPLL_OK;
        pC = pC->nextC;
    }
    return DPLL_FALSE;
}
int DPLL_unitClauseDiss(DPLL_Cnf *cnf){//单子句传播,不消除孤立文字
    if(DPLL_emptyCnf(*cnf))
        return DPLL_TRUE;
    if(cnf->lnum == 0)
        return DPLL_ERROR;
    int returnflag = 1;
    //Test int i = 0;
    while(1){
        //Testprintf("单子句传播第%d次循环\n",i);
        returnflag = DPLL_unitClauseDelete(cnf);
        //Testprintf("已完成单子句删除函数\n");
        if(returnflag == DPLL_FALSE){
            // 检查空CNF和空子句
            returnflag = DPLL_emptyCnf(*cnf);
            if (returnflag == DPLL_OK)
                return DPLL_TRUE;
            else if (returnflag == DPLL_ERROR)
                return DPLL_ERROR;
            returnflag = DPLL_wrongClause(*cnf);
            if (returnflag == DPLL_OK)
                return DPLL_WRONG;
            else if (returnflag == DPLL_ERROR)
                return DPLL_ERROR;
            return DPLL_FALSE; // 没有单子句也没有空子句
        }
        else if(returnflag == DPLL_ERROR){
            printf("单子句删除错误");
            return DPLL_ERROR;
        }
        else if(returnflag == DPLL_OK){
            //Testprintf("开始验空\n");
            returnflag = DPLL_emptyCnf(*cnf);
            if(returnflag == DPLL_OK)
                return DPLL_TRUE;
            else if(returnflag == DPLL_ERROR){
                printf("验空错误\n");
                return DPLL_ERROR;
            }
            returnflag = DPLL_wrongClause(*cnf);
            if(returnflag == DPLL_OK)
                return DPLL_WRONG;
            else if(returnflag == DPLL_ERROR)
            {
                printf("Cnf首子句为空指针\n");
                return DPLL_ERROR;
            }
        }
        // if(i > 100000)
        //     break;
    }
    printf("循环退出\n");
    return DPLL_ERROR;
}

int DPLL_DpllPro(DPLL_Cnf *cnf, int *backtime){
    int flag = 0;
    *backtime = 0;
    if(!cnf)
        return DPLL_ERROR;
    if(DPLL_emptyCnf(*cnf))
        return DPLL_OK;
    if(cnf->lnum == 0)
        return DPLL_ERROR;
    int backpoint = 0, changepoint = 0, returnflag = 0, order = 0;
    DPLL_Clause *pC = NULL; 
    int i = 0;   
    while(1){
        // if(i > 1000){
        //     printf("循环次数过多\n");
        //     return ERROR;
        // }
        // printf("第%d次循环\n",i++);
        flag = DPLL_unitClauseDiss(cnf);
        //printf("已完成单子句传播函数\n");
        if(flag == DPLL_TRUE)
            return DPLL_OK;
        else if(flag == DPLL_ERROR)
            return DPLL_ERROR;
        else if(flag == DPLL_WRONG){
            if(cnf->topA == NULL)
                return DPLL_FALSE;
            backpoint = cnf->topA->decsionPoint;
            changepoint = 0 - backpoint;
            flag = DPLL_backtrack(cnf);
            if(flag == DPLL_FALSE)
                return DPLL_FALSE;
            else if(flag == DPLL_ERROR){
                printf("回溯错误\n");
                return DPLL_ERROR;
            }
            pC = cnf->headC;
            while(pC){
                if(pC->size == 0){
                    printf("回溯发现错误子句\n");
                    return DPLL_ERROR;
                }
                returnflag = DPLL_isAtClause(changepoint, *pC);
                if(returnflag == DPLL_OK){
                    order = pC->order;
                    pC = pC->nextC;
                    returnflag = DPLL_removeClause(order, cnf);
                    if(returnflag == DPLL_ERROR){
                        printf("回溯删除子句失败\n");
                    return DPLL_ERROR;
                    }
                    continue;
                }
                returnflag = DPLL_isAtClause(backpoint, *pC);
                if(returnflag == DPLL_OK){
                    returnflag = DPLL_removeLiteral(backpoint, pC, cnf->topA);
                    if(returnflag == DPLL_ERROR){
                        printf("回溯文字失败\n");
                        return DPLL_ERROR;
                    }
                }
                pC = pC->nextC;
            }
            (*backtime)++;
             //printf("回溯次数：%d\n", *backtime);
            continue;
        }
        else if(flag == DPLL_FALSE){
             //printf("开始赋值\n");
             flag = DPLL_assignWithWeight(cnf);
              //printf("已完成赋值函数\n");
            if(flag == DPLL_OK){
                int emptyFlag = DPLL_emptyCnf(*cnf);
                if(emptyFlag == DPLL_OK)
                    return DPLL_OK;
                else if(emptyFlag == DPLL_ERROR){
                    printf("验空错误\n");
                    return DPLL_ERROR;
                }
                else if(emptyFlag == DPLL_FALSE)
                    continue;
            }
            else if(flag == DPLL_ERROR){
                printf("赋值错误\n");
                return DPLL_ERROR;
            }
            else if(flag == DPLL_TRUE)
                return DPLL_OK;            
        }
    }
}

int DPLL_generateResult(DPLL_Cnf cnf, char *outfile, double timeConsume, int result){
    FILE *fp = fopen(outfile, "w");
    if(!fp)
        return DPLL_ERROR;
    fprintf(fp, "s %d\n", result);
    fprintf(fp,"v");
    if(result == 0){
        fprintf(fp, " 0\n");
    }
    else{
        for(int i = 1; i <= cnf.lnum; i++){
            if(cnf.literals[i] == 1)
                fprintf(fp, " %d", i);
            else if(cnf.literals[i] == 0)
                fprintf(fp, " -%d", i);
        }
    }
    fprintf(fp, "\nt %lf", timeConsume);
    fclose(fp);
    return DPLL_OK;
}

