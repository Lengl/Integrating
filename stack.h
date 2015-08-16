#define STACK_MAX_TASK 100000

typedef struct {
	double A, B, fA, fB, sAB;
} record;

typedef struct {
// 	record *stk;
	record stk[STACK_MAX_TASK];
	unsigned int stTop;
} stack;

void stackInit(stack *stck) {
	stck->stTop = 0;
// 	stck->stk = (record *)malloc((size + 1) * sizeof(record));
	
}

void stackDestroy(stack *stck) {
	free(stck->stk);
	stck->stTop = -1;
}

void printRecord(record rec) {
	fprintf(stdout, "A = %6f, fA = %6f, B = %6f, fB = %6f, sAB = %6f\n", rec.A, rec.fA, rec.B, rec.fB, rec.sAB);
}

void putIntoStack(stack *stck, record *rec) {
	unsigned int top = stck->stTop;
	record *topRec = &(stck->stk[top]);
	topRec->A = rec->A;
	topRec->B = rec->B;
	topRec->fA = rec->fA;
	topRec->fB = rec->fB;
	topRec->sAB = rec->sAB;
	(stck->stTop)++;
}

void getFromStack(stack *stck, record *rec) {
	if (stck->stTop == 0)
		return;
	unsigned int top = --(stck->stTop);
	record *topRec = &(stck->stk[top]);
	rec->A = topRec->A;
	rec->B = topRec->B;
	rec->fA = topRec->fA;
	rec->fB = topRec->fB;
	rec->sAB = topRec->sAB;
}