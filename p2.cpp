#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

//#ifdef __APPLE__

//#else
//#include <CL/cl.h>
//#endif

#define MAX_SOURCE_SIZE (0x100000)
#define NGRAM 7
#define CLASSNUM 8
#define MAX_LINE_SIZE 0x100000
#define DIMEN 10000


int * RandPerm(int d)
{
    int *perm;
    perm = (int *)malloc(d * sizeof(int));
    
    for (int i = 0; i < d; i++) perm[i] = i;
    
    for (int i = 0; i < d; i++) {
        int j, t;
        j = rand() % (d - i) + i;
        t = perm[j];
        perm[j] = perm[i];
        perm[i] = t; // Swap i and j with t
    }
    return perm;
    
}

int * BinarizeHV(int *val, int D)
{
    int threshold = 0;
    for (int i = 0; i < D; i++)
    {
        if (val[i] > threshold)
            val[i] = 1;
        else
            val[i] = -1;
    }
    return val;
}

int * GetRandomHV(int D)
{
    int* perm = RandPerm(D);
    
    int *RandomHV = (int*)malloc(sizeof(int)*D);
    
    for (int i = 0; i < D; i++)
    {
        int index = perm[i];
        if (i < D / 2)
            RandomHV[index] = 1;
        else
            RandomHV[index] = -1;
    }
    return RandomHV;
}

int GetLetterIndex(char buf) {
    char s[] = "abcdefghijklmnopqrstuvwxyz ";
    for (int j = 0; j < strlen(s); j++) {
        if (s[j] == buf) {
            return j;
        }
    }
    return -1;
}

char* GetlangLabels(const int keyindex)
{
    char *langLabels[] = { "acq", "crude", "earn", "grain", "interest", "money-fx", "ship", "trade" };
    
    return langLabels[keyindex];
}

int GetlangLabelsFileCount(const int keyindex)
{
    int langLabelsFileCount[] = { 696, 121, 1083, 10, 81, 87, 36, 75 };
    
    return langLabelsFileCount[keyindex];
}



char * GetClassFromString(char* str, char *key, int *diff)
{
    int index = 0;
    
    
    for (int i = 0; i < 10; i++)
    {
        if (str[i] == ' ' || str[i] == '\t')
        {
            index = i;
            break;
        }
    }
    if (index == 0) return NULL;
    else {
        
        strncpy(key, str, index);
        
        *diff = index;
        key[index] = '\0';
        return key;
    }
}


void AddVector(int *out, int* in, int D)
{
    for (int i = 0; i < D; i++)
    {
        out[i] += in[i];
    }
}

void MulVector(int *out, int* in, int D)
{
    for (int i = 0; i < D; i++)
    {
        out[i] *= in[i];
    }
}

void ComputeSumHV(int* langAM, char* buffer, int** IM, int N, int D)
{
    int* block[NGRAM];
    int* blockt[NGRAM];
    
    for (int i = 0; i < NGRAM; i++)
    {
        block[i] = (int*)malloc(sizeof(int) * DIMEN);
        blockt[i] = (int*)malloc(sizeof(int) * DIMEN);
        for (int j = 0; j < DIMEN; j++)
        {
            block[i][j] = 0;
            blockt[i][j] = 0;
        }
    }
    
    
    int index = 0;
    for (int numItems = 0; numItems < strlen(buffer); numItems++)
    {
        
        char key = buffer[numItems];
        int keynum = GetLetterIndex(key);
        if (keynum == -1) continue;
        
        // circle shift of block
        
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < D; j++)
            {
                blockt[i][j] = block[i][j];
            }
        }
        
        for (int i = 0; i < N; i++) {
            int ii = (i + 1) % N;
            if (ii<0) ii = N + ii;
            for (int j = 0; j < D; j++) {
                int jj = (j + 1) % D;
                if (jj<0) jj = D + jj;
                block[ii][jj] = blockt[i][j];
            }
        }
        
        for (int i = 0; i < D; i++)
        {
            block[0][i] = IM[keynum][i];
        }
        
        int nGrams[DIMEN] = { 0 };
        
        if (index >= N)
        {
            for (int i = 0; i < D; i++)
                nGrams[i] = block[0][i];
            
            for (int i = 1; i < N; i++)
            {
                //AddVector(nGrams, block[i], DIMEN); //New Encoder
                MulVector(nGrams, block[i], DIMEN); //original Algorithm
            }
            
            AddVector(langAM, nGrams, DIMEN);
            
        }
        index++;
        
    }
    for (int i = 0; i < NGRAM; i++)
    {
        free(block[i]);
        free(blockt[i]);
    }
    
    return;
}



float norm(int* u, int D)
{
    float sum = 0;
    for (int i = 0; i < D; i++)
        sum += (float)u[i] * (float)u[i];
    
    return (float)sqrt(sum);
}

float cosAngle(int* u, int* v, int D)
{
    int value = 0;
    for (int i = 0; i < D; i++)
    {
        value += u[i] * v[i];
    }
    return (float)value / (norm(u, D)* norm(v, D));
}


char* itoa(int num)
{
    /* log10(num) gives the number of digits; + 1 for the null terminator */
    if (num==0) {
        return "0";
    }
    int size = log10(num) + 1;
    char *str = malloc(size);
    int i, rem, len = 0, n;
    
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
    
    return str;
}

int main(void) {

    //// --- ItemMemory --- ///////
    
    char s[] = "abcdefghijklmnopqrstuvwxyz ";
    int  *itemMemory[27];   // itemMemory
    
    // Make ItemMemory with RandomHV
    for (int i = 0; i < strlen(s); i++) {
        itemMemory[i] = GetRandomHV(DIMEN);
    }
    
    //////// ---- Read Train Data ----- //////////
//    FILE *ptr_file;
  //  ptr_file=fopen("/Users/bhavin/Downloads/r8-train-all-terms.txt", "r");
   // printf("Read training file...\n");
    
 //   if (!ptr_file) {
       // fprintf(stderr, "Failed to load file.\n");
 //       exit(1);
  //  }
    
    char* linestr = (char*)malloc(MAX_LINE_SIZE);
    
    
    int *langAM[CLASSNUM];
    for (int i = 0; i < CLASSNUM; i++) {
        langAM[i] = (int*)malloc(sizeof(int)*DIMEN);
        for (int kk = 0; kk < DIMEN; kk++)
            langAM[i][kk] = 0;
    }
    printf("\n Start training......\n");
    char * path= "/Users/bhavin/Downloads/Data/train/";
    for (int i; i<CLASSNUM; i++) {
        char *filelocation= (char *)calloc(150, sizeof(char));
        strcat(filelocation, path);
        strcat(filelocation, GetlangLabels(i));
        strcat(filelocation, ".txt");
        FILE *ptr_file;
        ptr_file=fopen(filelocation, "r");
        // printf("Read training file...\n");
        
        if (!ptr_file) {
            // fprintf(stderr, "Failed to load file.\n");
            exit(1);
        }

        

    while (fgets(linestr, MAX_LINE_SIZE, ptr_file) != NULL)
    {
        ComputeSumHV(langAM[i], linestr, itemMemory, NGRAM, DIMEN);
    }
        printf("\n done trained. of file %s\n",filelocation);
        fclose(ptr_file);
    }
    
    printf("\n All trained.\n");
    
    for (int i = 0; i < CLASSNUM; i++)
    {
        BinarizeHV(langAM[i], DIMEN);
    }
    
    
    ////// ----- test --------- ////////

    char* tlinestr = (char*)malloc(MAX_LINE_SIZE);
    int correct = 0;
    int total = 0;
    
    printf("Start testing...\n");
    char * testPath= "/Users/bhavin/Downloads/Data/test/";
    for (int i=0; i<CLASSNUM; i++) {
        for (int j=0; j<GetlangLabelsFileCount(i); j++) {
        char *filelocation= (char *)calloc(150, sizeof(char));
        strcat(filelocation, testPath);
        strcat(filelocation, GetlangLabels(i));
        strcat(filelocation,itoa(j));
        strcat(filelocation, ".txt");
        FILE *test_file;
        test_file=fopen(filelocation, "r");
        // printf("Read training file...\n");
        
        if (!test_file) {
            // fprintf(stderr, "Failed to load file.\n");
            exit(1);
        }
        
        

    bool flag = false;
    while (fgets(tlinestr, MAX_LINE_SIZE, test_file) != NULL)
    {
        char key[10];
        int *predicHV = (int*)malloc(sizeof(int)*DIMEN);
        for (int i = 0; i < DIMEN; i++)
            predicHV[i] = 0;
        //ComputeSumHVWithKernel(context, command_queue, kernel_add, kernel_mul, predicHV, tlinestr, itemMemory, NGRAM, DIMEN);
        ComputeSumHV(predicHV, tlinestr, itemMemory, NGRAM, DIMEN);
        BinarizeHV(predicHV, DIMEN);
        float maxAngle = -1.0;
        int predictedValue = 0;
        for (int i = 0; i < CLASSNUM; i++)
        {
            float angle = cosAngle(langAM[i], predicHV, DIMEN);
            
            if (angle > maxAngle)
            {
                maxAngle = angle;
                predictedValue = i;
            }
        }
        flag = false;
        if (predictedValue == i)
        {
            correct = correct + 1;
            flag = true;
        }
        total = total + 1;
        if (flag==false) {
            printf("\n done trained. of file %s\n",filelocation);
        }
        
    printf("\n%10s, %5s, %5d,  %5d,  %5.3f", GetlangLabels(i), (flag ? "true" : "false"), total, correct, (float)(correct) / (float)(total));
    }
        fclose(test_file);
    }
    }
    float accuracy = (float)(correct) / (float)(total);
   printf("\nAccuracy = %5.3f", accuracy * 100);
    
    
    
    int ii;
    scanf("%d", &ii);
    return 0;
}
