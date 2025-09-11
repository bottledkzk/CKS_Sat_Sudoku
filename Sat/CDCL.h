#include "SatHead.h"

// 字句结构体
typedef struct {
    int lbd;            // 字句的LBD值
    int size;           // 字句中的文字数量
    int* lits;          // 文字数组
} Clause;

// 观察器结构体
typedef struct {
    int idx_clause;     // 字句在数据库中的索引
    int blocker;        // 阻塞文字
} Watcher;

// 观察器列表结构体
typedef struct {
    Watcher* array;     // 观察器数组
    int size;           // 当前大小
    int capacity;       // 容量
} WatcherList;

// 堆结构体
typedef struct {
    int* heap;          // 堆数组
    int* pos;           // 位置数组
    int size;           // 堆大小
    int capacity;       // 堆容量
    double* activity;   // 活动度数组
} Heap;

// 求解器结构体
typedef struct {
    // 问题基本信息
    int vars;           // 变量数量
    int clauses;        // 子句数量
    int origin_clauses; // 原始子句数量
    
    // 数据结构
    Clause* clause_DB;      // 子句数据库
    int clause_DB_size;     // 子句数据库当前大小
    int clause_DB_capacity; // 子句数据库容量
    
    WatcherList* watches;   // 观察器列表数组
    
    // 赋值相关
    int* value;         // 变量赋值 (1:真, -1:假, 0:未赋值)
    int* reason;        // 推导原因（子句索引）
    int* level;         // 决策层级
    int* mark;          // 标记数组（用于分析）
    int* local_best;    // 局部最佳相位
    int* saved;         // 保存的相位
    
    // 活动度相关
    double* activity;   // 变量活动度
    double var_inc;     // 活动度增量
    
    // 堆
    Heap vsids;         // VSIDS决策堆
    
    // 跟踪和传播
    int* trail;         // 赋值轨迹
    int trail_size;     // 轨迹大小
    int trail_capacity; // 轨迹容量
    
    int* pos_in_trail;  // 决策点在轨迹中的位置
    int pos_in_trail_size;     // 决策点数量
    int pos_in_trail_capacity; // 决策点容量
    
    int propagated;     // 已传播的文字数量
    
    // 学习相关
    int* learnt;        // 学习到的子句
    int learnt_size;    // 学习到的子句大小
    int learnt_capacity; // 学习到的子句容量
    
    // 统计信息
    int conflicts;      // 冲突次数
    int restarts;       // 重启次数
    int rephases;       // 重新相位次数
    int reduces;        // 子句清理次数
    
    int rephase_limit;  // 重新相位限制
    int reduce_limit;   // 子句清理限制
    int threshold;      // 阈值
    
    int time_stamp;     // 时间戳（用于分析）
    
    // LBD相关
    int lbd_queue[50];  // LBD队列
    int lbd_queue_size; // LBD队列大小
    int lbd_queue_pos;  // LBD队列位置
    double fast_lbd_sum; // 快速LBD和
    double slow_lbd_sum; // 慢速LBD和
    
    // 缩减映射
    int* reduce_map;    // 子句缩减映射
    int reduce_map_size; // 缩减映射大小
} Solver;

// 辅助函数：读取空白字符
char* read_whitespace(char* p) {
    while ((*p >= 9 && *p <= 13) || *p == 32) ++p;
    return p;
}

// 辅助函数：读取直到换行符
char* read_until_new_line(char* p) {
    while (*p != '\n') {
        if (*p++ == '\0') exit(1);
    }
    return ++p;
}

// 辅助函数：读取整数
char* read_int(char* p, int* i) {
    bool sym = true;
    *i = 0;
    p = read_whitespace(p);
    if (*p == '-') {
        sym = false;
        ++p;
    }
    while (*p >= '0' && *p <= '9') {
        if (*p == '\0') return p;
        *i = *i * 10 + *p - '0';
        ++p;
    }
    if (!sym) *i = -(*i);
    return p;
}

// 初始化观察器列表
void init_watcher_list(WatcherList* list) {
    list->array = NULL;
    list->size = 0;
    list->capacity = 0;
}

// 向观察器列表添加观察器
void watcher_list_push(WatcherList* list, Watcher watcher) {
    if (list->size >= list->capacity) {
        int new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
        list->array = (Watcher*)realloc(list->array, new_capacity * sizeof(Watcher));
        list->capacity = new_capacity;
    }
    list->array[list->size++] = watcher;
}

// 释放观察器列表
void free_watcher_list(WatcherList* list) {
    if (list->array) free(list->array);
    list->array = NULL;
    list->size = list->capacity = 0;
}

// 初始化堆
void heap_init(Heap* heap, double* activity, int capacity) {
    heap->heap = (int*)malloc(capacity * sizeof(int));
    heap->pos = (int*)malloc(capacity * sizeof(int));
    heap->size = 0;
    heap->capacity = capacity;
    heap->activity = activity;
    
    for (int i = 0; i < capacity; i++) {
        heap->pos[i] = -1;
    }
}

// 释放堆
void heap_free(Heap* heap) {
    free(heap->heap);
    free(heap->pos);
    heap->heap = NULL;
    heap->pos = NULL;
    heap->size = heap->capacity = 0;
}

// 堆上浮操作
void heap_up(Heap* heap, int v) {
    int x = heap->heap[v];
    int p = (v - 1) >> 1; // father
    
    while (v && heap->activity[x] > heap->activity[heap->heap[p]]) {
        heap->heap[v] = heap->heap[p];
        heap->pos[heap->heap[p]] = v;
        v = p;
        p = (p - 1) >> 1;
    }
    heap->heap[v] = x;
    heap->pos[x] = v;
}

// 堆下沉操作
void heap_down(Heap* heap, int v) {
    int x = heap->heap[v];
    
    while (1) {
        int left = (v << 1) | 1;
        int right = (v + 1) << 1;
        
        if (left >= heap->size) break;
        
        int child = left;
        if (right < heap->size && heap->activity[heap->heap[right]] > heap->activity[heap->heap[left]]) {
            child = right;
        }
        
        if (heap->activity[heap->heap[child]] <= heap->activity[x]) break;
        
        heap->heap[v] = heap->heap[child];
        heap->pos[heap->heap[v]] = v;
        v = child;
    }
    
    heap->heap[v] = x;
    heap->pos[x] = v;
}

// 检查堆是否为空
bool heap_empty(Heap* heap) {
    return heap->size == 0;
}

// 检查元素是否在堆中
bool heap_in_heap(Heap* heap, int n) {
    return n < heap->capacity && heap->pos[n] >= 0;
}

// 更新堆中元素
void heap_update(Heap* heap, int x) {
    if (heap_in_heap(heap, x)) {
        heap_up(heap, heap->pos[x]);
    }
}

// 向堆中插入元素
void heap_insert(Heap* heap, int x) {
    if (x >= heap->capacity) {
        int new_capacity = heap->capacity * 2;
        while (new_capacity <= x) new_capacity *= 2;
        
        heap->heap = (int*)realloc(heap->heap, new_capacity * sizeof(int));
        heap->pos = (int*)realloc(heap->pos, new_capacity * sizeof(int));
        
        for (int i = heap->capacity; i < new_capacity; i++) {
            heap->pos[i] = -1;
        }
        
        heap->capacity = new_capacity;
    }
    
    heap->pos[x] = heap->size;
    heap->heap[heap->size] = x;
    heap_up(heap, heap->size);
    heap->size++;
}

// 从堆中弹出元素
int heap_pop(Heap* heap) {
    int x = heap->heap[0];
    heap->pos[x] = -1;
    
    heap->size--;
    if (heap->size > 0) {
        heap->heap[0] = heap->heap[heap->size];
        heap->pos[heap->heap[0]] = 0;
        heap_down(heap, 0);
    }
    
    return x;
}

// 获取文字的值
int value(Solver* s, int lit) {
    int var = abs(lit);
    return lit > 0 ? s->value[var] : -s->value[var];
}

// 获取观察器列表
WatcherList* watch(Solver* s, int lit) {
    return &s->watches[s->vars + lit];
}

// 初始化求解器
void init_solver(Solver* s) {
    memset(s, 0, sizeof(Solver));
    s->var_inc = 1.0;
    s->rephase_limit = 1024;
    s->reduce_limit = 8192;
}

// 分配内存
void alloc_memory(Solver* s) {
    s->value = (int*)calloc(s->vars + 1, sizeof(int));
    s->reason = (int*)calloc(s->vars + 1, sizeof(int));
    s->level = (int*)calloc(s->vars + 1, sizeof(int));
    s->mark = (int*)calloc(s->vars + 1, sizeof(int));
    s->local_best = (int*)calloc(s->vars + 1, sizeof(int));
    s->saved = (int*)calloc(s->vars + 1, sizeof(int));
    s->activity = (double*)calloc(s->vars + 1, sizeof(double));
    
    s->watches = (WatcherList*)calloc(s->vars * 2 + 1, sizeof(WatcherList));
    for (int i = 0; i < s->vars * 2 + 1; i++) {
        init_watcher_list(&s->watches[i]);
    }
    
    heap_init(&s->vsids, s->activity, s->vars + 1);
    for (int i = 1; i <= s->vars; i++) {
        heap_insert(&s->vsids, i);
    }
    
    s->trail_capacity = s->vars * 2;
    s->trail = (int*)malloc(s->trail_capacity * sizeof(int));
    s->trail_size = 0;
    
    s->pos_in_trail_capacity = s->vars;
    s->pos_in_trail = (int*)malloc(s->pos_in_trail_capacity * sizeof(int));
    s->pos_in_trail_size = 0;
    
    s->learnt_capacity = s->vars;
    s->learnt = (int*)malloc(s->learnt_capacity * sizeof(int));
    s->learnt_size = 0;
    
    s->clause_DB_capacity = s->clauses * 2;
    s->clause_DB = (Clause*)malloc(s->clause_DB_capacity * sizeof(Clause));
    s->clause_DB_size = 0;
    
    s->reduce_map_size = s->clause_DB_capacity;
    s->reduce_map = (int*)malloc(s->reduce_map_size * sizeof(int));
}

// 释放求解器
void free_solver(Solver* s) {
    if (s->value) free(s->value);
    if (s->reason) free(s->reason);
    if (s->level) free(s->level);
    if (s->mark) free(s->mark);
    if (s->local_best) free(s->local_best);
    if (s->saved) free(s->saved);
    if (s->activity) free(s->activity);
    
    if (s->watches) {
        for (int i = 0; i < s->vars * 2 + 1; i++) {
            free_watcher_list(&s->watches[i]);
        }
        free(s->watches);
    }
    
    heap_free(&s->vsids);
    
    if (s->trail) free(s->trail);
    if (s->pos_in_trail) free(s->pos_in_trail);
    if (s->learnt) free(s->learnt);
    
    if (s->clause_DB) {
        for (int i = 0; i < s->clause_DB_size; i++) {
            if (s->clause_DB[i].lits) free(s->clause_DB[i].lits);
        }
        free(s->clause_DB);
    }
    
    if (s->reduce_map) free(s->reduce_map);
}

// 增加子句到数据库
int add_clause(Solver* s, int* lits, int size) {
    if (s->clause_DB_size >= s->clause_DB_capacity) {
        s->clause_DB_capacity *= 2;
        s->clause_DB = (Clause*)realloc(s->clause_DB, s->clause_DB_capacity * sizeof(Clause));
    }
    
    Clause* c = &s->clause_DB[s->clause_DB_size];
    c->size = size;
    c->lits = (int*)malloc(size * sizeof(int));
    memcpy(c->lits, lits, size * sizeof(int));
    c->lbd = 0;
    
    int id = s->clause_DB_size;
    s->clause_DB_size++;
    
    // 添加观察器
    Watcher w1 = {id, lits[1]};
    watcher_list_push(watch(s, -lits[0]), w1);
    
    Watcher w2 = {id, lits[0]};
    watcher_list_push(watch(s, -lits[1]), w2);
    
    return id;
}

// 变量活动度提升
void bump_var(Solver* s, int var, double coeff) {
    s->activity[var] += s->var_inc * coeff;
    
    // 防止浮点数溢出
    if (s->activity[var] > 1e100) {
        for (int i = 1; i <= s->vars; i++) {
            s->activity[i] *= 1e-100;
        }
        s->var_inc *= 1e-100;
    }
    
    if (heap_in_heap(&s->vsids, var)) {
        heap_update(&s->vsids, var);
    }
}

// 赋值变量
void assign(Solver* s, int lit, int l, int cref) {
    int var = abs(lit);
    s->value[var] = lit > 0 ? 1 : -1;
    s->level[var] = l;
    s->reason[var] = cref;
    
    if (s->trail_size >= s->trail_capacity) {
        s->trail_capacity *= 2;
        s->trail = (int*)realloc(s->trail, s->trail_capacity * sizeof(int));
    }
    s->trail[s->trail_size++] = lit;
}

// 传播
int propagate(Solver* s) {
    while (s->propagated < s->trail_size) {
        int p = s->trail[s->propagated++];
        WatcherList* ws = watch(s, p);
        
        int i = 0, j = 0;
        while (i < ws->size) {
            int blocker = ws->array[i].blocker;
            if (value(s, blocker) == 1) {
                ws->array[j++] = ws->array[i++];
                continue;
            }
            
            int cref = ws->array[i].idx_clause;
            Clause* c = &s->clause_DB[cref];
            
            if (c->lits[0] == -p) {
                // 交换前两个文字
                int temp = c->lits[0];
                c->lits[0] = c->lits[1];
                c->lits[1] = temp;
            }
            
            Watcher w = {cref, c->lits[0]};
            i++;
            
            if (value(s, c->lits[0]) == 1) {
                ws->array[j++] = w;
                continue;
            }
            
            int k;
            for (k = 2; k < c->size && value(s, c->lits[k]) == -1; k++);
            
            if (k < c->size) {
                // 找到新的观察文字
                c->lits[1] = c->lits[k];
                c->lits[k] = -p;
                watcher_list_push(watch(s, -c->lits[1]), w);
            } else {
                // 无法找到新的观察文字
                ws->array[j++] = w;
                
                if (value(s, c->lits[0]) == -1) {
                    // 冲突
                    while (i < ws->size) {
                        ws->array[j++] = ws->array[i++];
                    }
                    ws->size = j;
                    return cref;
                } else {
                    // 单元传播
                    assign(s, c->lits[0], s->level[abs(p)], cref);
                }
            }
        }
        ws->size = j;
    }
    return -1;
}

// 冲突分析
int analyze(Solver* s, int conflict, int* backtrackLevel, int* lbd) {
    s->time_stamp++;
    s->learnt_size = 0;
    
    Clause* c = &s->clause_DB[conflict];
    int highestLevel = s->level[abs(c->lits[0])];
    if (highestLevel == 0) return 20;
    
    // 为第一个UIP留位置
    if (s->learnt_size >= s->learnt_capacity) {
        s->learnt_capacity *= 2;
        s->learnt = (int*)realloc(s->learnt, s->learnt_capacity * sizeof(int));
    }
    s->learnt[s->learnt_size++] = 0;
    
    int bump_size = 0;
    int bump_capacity = 10;
    int* bump = (int*)malloc(bump_capacity * sizeof(int));
    
    int should_visit_ct = 0;
    int resolve_lit = 0;
    int index = s->trail_size - 1;
    
    do {
        c = &s->clause_DB[conflict];
        int start = (resolve_lit == 0) ? 0 : 1;
        
        for (int i = start; i < c->size; i++) {
            int var = abs(c->lits[i]);
            if (s->mark[var] != s->time_stamp && s->level[var] > 0) {
                bump_var(s, var, 0.5);
                
                if (bump_size >= bump_capacity) {
                    bump_capacity *= 2;
                    bump = (int*)realloc(bump, bump_capacity * sizeof(int));
                }
                bump[bump_size++] = var;
                
                s->mark[var] = s->time_stamp;
                if (s->level[var] >= highestLevel) {
                    should_visit_ct++;
                } else {
                    if (s->learnt_size >= s->learnt_capacity) {
                        s->learnt_capacity *= 2;
                        s->learnt = (int*)realloc(s->learnt, s->learnt_capacity * sizeof(int));
                    }
                    s->learnt[s->learnt_size++] = c->lits[i];
                }
            }
        }
        
        do {
            while (s->mark[abs(s->trail[index--])] != s->time_stamp);
            resolve_lit = s->trail[index + 1];
        } while (s->level[abs(resolve_lit)] < highestLevel);
        
        conflict = s->reason[abs(resolve_lit)];
        s->mark[abs(resolve_lit)] = 0;
        should_visit_ct--;
    } while (should_visit_ct > 0);
    
    s->learnt[0] = -resolve_lit;
    
    s->time_stamp++;
    *lbd = 0;
    
    for (int i = 0; i < s->learnt_size; i++) {
        int l = s->level[abs(s->learnt[i])];
        if (l && s->mark[l] != s->time_stamp) {
            s->mark[l] = s->time_stamp;
            (*lbd)++;
        }
    }
    
    if (s->lbd_queue_size < 50) s->lbd_queue_size++;
    else s->fast_lbd_sum -= s->lbd_queue[s->lbd_queue_pos];
    
    s->fast_lbd_sum += *lbd;
    s->lbd_queue[s->lbd_queue_pos++] = *lbd;
    if (s->lbd_queue_pos == 50) s->lbd_queue_pos = 0;
    
    s->slow_lbd_sum += (*lbd > 50 ? 50 : *lbd);
    
    if (s->learnt_size == 1) {
        *backtrackLevel = 0;
    } else {
        int max_id = 1;
        for (int i = 2; i < s->learnt_size; i++) {
            if (s->level[abs(s->learnt[i])] > s->level[abs(s->learnt[max_id])]) {
                max_id = i;
            }
        }
        
        int p = s->learnt[max_id];
        s->learnt[max_id] = s->learnt[1];
        s->learnt[1] = p;
        *backtrackLevel = s->level[abs(p)];
    }
    
    for (int i = 0; i < bump_size; i++) {
        if (s->level[bump[i]] >= *backtrackLevel - 1) {
            bump_var(s, bump[i], 1.0);
        }
    }
    
    free(bump);
    return 0;
}

// 回溯
void backtrack(Solver* s, int backtrackLevel) {
    if (s->pos_in_trail_size <= backtrackLevel) return;
    
    for (int i = s->trail_size - 1; i >= s->pos_in_trail[backtrackLevel]; i--) {
        int v = abs(s->trail[i]);
        s->value[v] = 0;
        s->saved[v] = s->trail[i] > 0 ? 1 : -1;
        
        if (!heap_in_heap(&s->vsids, v)) {
            heap_insert(&s->vsids, v);
        }
    }
    
    s->propagated = s->pos_in_trail[backtrackLevel];
    s->trail_size = s->propagated;
    s->pos_in_trail_size = backtrackLevel;
}

// 决策
int decide(Solver* s) {
    int next = -1;
    
    while (next == -1 || s->value[next] != 0) {
        if (heap_empty(&s->vsids)) return 10;
        next = heap_pop(&s->vsids);
    }
    
    if (s->pos_in_trail_size >= s->pos_in_trail_capacity) {
        s->pos_in_trail_capacity *= 2;
        s->pos_in_trail = (int*)realloc(s->pos_in_trail, s->pos_in_trail_capacity * sizeof(int));
    }
    s->pos_in_trail[s->pos_in_trail_size++] = s->trail_size;
    
    if (s->saved[next]) {
        next *= s->saved[next];
    }
    
    assign(s, next, s->pos_in_trail_size, -1);
    return 0;
}

// 重启
void restart(Solver* s) {
    s->fast_lbd_sum = 0;
    s->lbd_queue_size = 0;
    s->lbd_queue_pos = 0;
    
    backtrack(s, 0);
    
    int phase_rand = rand() % 100;
    
    if ((phase_rand -= 60) < 0) {
        for (int i = 1; i <= s->vars; i++) {
            s->saved[i] = s->local_best[i];
        }
    } else if ((phase_rand -= 5) < 0) {
        for (int i = 1; i <= s->vars; i++) {
            s->saved[i] = -s->local_best[i];
        }
    } else if ((phase_rand -= 20) < 0) {
        for (int i = 1; i <= s->vars; i++) {
            s->saved[i] = (rand() % 2) ? 1 : -1;
        }
    }
}

// 重新相位
void rephase(Solver* s) {
    s->rephases = 0;
    s->threshold *= 0.9;
    s->rephase_limit += 8192;
}

// 子句清理
void reduce(Solver* s) {
    backtrack(s, 0);
    s->reduces = 0;
    s->reduce_limit += 512;
    
    int new_size = s->origin_clauses;
    int old_size = s->clause_DB_size;
    
    if (s->reduce_map_size < old_size) {
        s->reduce_map_size = old_size;
        s->reduce_map = (int*)realloc(s->reduce_map, s->reduce_map_size * sizeof(int));
    }
    
    for (int i = s->origin_clauses; i < old_size; i++) {
        if (s->clause_DB[i].lbd >= 5 && rand() % 2 == 0) {
            s->reduce_map[i] = -1;
            free(s->clause_DB[i].lits);
        } else {
            if (new_size != i) {
                s->clause_DB[new_size] = s->clause_DB[i];
            }
            s->reduce_map[i] = new_size++;
        }
    }
    
    s->clause_DB_size = new_size;
    
    for (int v = -s->vars; v <= s->vars; v++) {
        if (v == 0) continue;
        
        WatcherList* wl = watch(s, v);
        int new_sz = 0;
        
        for (int i = 0; i < wl->size; i++) {
            int old_idx = wl->array[i].idx_clause;
            int new_idx = old_idx < s->origin_clauses ? old_idx : s->reduce_map[old_idx];
            
            if (new_idx != -1) {
                wl->array[i].idx_clause = new_idx;
                if (new_sz != i) {
                    wl->array[new_sz] = wl->array[i];
                }
                new_sz++;
            }
        }
        
        wl->size = new_sz;
    }
}

// 解析CNF文件
int parse(Solver* s, char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Cannot open file: %s\n", filename);
        return 1;
    }
    
    // 跳过注释行，直到找到p行
    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == 'c') {
            // 跳过注释行
            while ((ch = fgetc(fp)) != EOF && ch != '\n');
        } else if (ch == 'p') {
            break;
        } else if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            printf("PARSE ERROR! Unexpected char: %c\n", ch);
            fclose(fp);
            return 1;
        }
    }
    
    // 检查范式是否为cnf
    char paradigm[10];
    if (fscanf(fp, "%s", paradigm) != 1 || strcmp(paradigm, "cnf") != 0) {
        printf("PARSE ERROR! Not a CNF file\n");
        fclose(fp);
        return 1;
    }
    
    // 读取变量数和子句数
    if (fscanf(fp, "%d %d", &s->vars, &s->clauses) != 2) {
        printf("PARSE ERROR! Cannot read variable and clause counts\n");
        fclose(fp);
        return 1;
    }
    
    // 分配内存
    alloc_memory(s);
    
    // 读取子句
    int literal;
    int buffer_size = 0;
    int buffer_capacity = 16;
    int* buffer = (int*)malloc(buffer_capacity * sizeof(int));
    int clauses_read = 0;
    
    while (fscanf(fp, "%d", &literal) != EOF && clauses_read < s->clauses) {
        if (literal == 0) {
            // 子句结束
            if (buffer_size == 0) {
                printf("PARSE ERROR! Empty clause\n");
                free(buffer);
                fclose(fp);
                return 1;
            }
            
            // 处理单文字子句
            if (buffer_size == 1) {
                if (value(s, buffer[0]) == -1) {
                    free(buffer);
                    fclose(fp);
                    return 20; // 冲突
                }
                
                if (!value(s, buffer[0])) {
                    assign(s, buffer[0], 0, -1);
                }
            } else {
                // 添加多文字子句
                add_clause(s, buffer, buffer_size);
            }
            
            buffer_size = 0;
            clauses_read++;
        } else {
            // 添加文字到缓冲区
            if (buffer_size >= buffer_capacity) {
                buffer_capacity *= 2;
                buffer = (int*)realloc(buffer, buffer_capacity * sizeof(int));
            }
            buffer[buffer_size++] = literal;
        }
    }
    
    free(buffer);
    fclose(fp);
    
    // 检查是否读取了所有子句
    if (clauses_read != s->clauses) {
        printf("PARSE ERROR! Expected %d clauses, read %d\n", s->clauses, clauses_read);
        return 1;
    }
    
    s->origin_clauses = s->clause_DB_size;
    return (propagate(s) == -1 ? 0 : 20);
}

// 求解
int solve(Solver* s) {
    int res = 0;
    
    while (!res) {
        int cref = propagate(s);
        
        if (cref != -1) {
            int backtrackLevel = 0, lbd = 0;
            res = analyze(s, cref, &backtrackLevel, &lbd);
            
            if (res == 20) break;
            
            backtrack(s, backtrackLevel);
            
            if (s->learnt_size == 1) {
                assign(s, s->learnt[0], 0, -1);
            } else {
                int new_cref = add_clause(s, s->learnt, s->learnt_size);
                s->clause_DB[new_cref].lbd = lbd;
                assign(s, s->learnt[0], backtrackLevel, new_cref);
            }
            
            s->var_inc *= 1.25; // 1/0.8 = 1.25
            
            s->restarts++;
            s->conflicts++;
            s->rephases++;
            s->reduces++;
            
            if (s->trail_size > s->threshold) {
                s->threshold = s->trail_size;
                for (int i = 1; i <= s->vars; i++) {
                    s->local_best[i] = s->value[i];
                }
            }
        } else if (s->reduces >= s->reduce_limit) {
            reduce(s);
        } else if (s->lbd_queue_size == 50 && 
                  0.8 * s->fast_lbd_sum / s->lbd_queue_size > s->slow_lbd_sum / s->conflicts) {
            restart(s);
        } else if (s->rephases >= s->rephase_limit) {
            rephase(s);
        } else {
            res = decide(s);
        }
    }
    
    return res;
}

// 打印模型
void print_model(Solver* s, FILE* fout) {
    fprintf(fout, "v ");
    for (int i = 1; i <= s->vars; i++) {
        fprintf(fout, "%d ", s->value[i] > 0 ? i : -i);
    }
    fprintf(fout, "0\n");
}