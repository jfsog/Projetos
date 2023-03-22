#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#define TAM 300
#define DESLOCAMEM 1000000
#define CASADEC 2
#define PRECISAO ((int)pow(10,CASADEC))
//Operaçôes de entrada/saída
#define READ 10            ///Lê uma palavra do terminal e a coloca em um local da memória
#define WRITE 11           ///Printa o valor contido em uma região da memória
#define STR_PRINT 12          ///Printa uma string
//Operaçôes de carregamento/armazenamento
#define LOAD 20            ///Carrega no acumulador uma palavra de um local específico da memória
#define STORE 21           ///Armazena em um local específico da memória uma palavra do acumulador
//Operações aritméticas
#define ADD 30             ///Adiciona ao acumulador um valor de uma região específica da memória
#define SUBTRACT 31        ///Subtrai do acumulador um valor de uma região específica da memória
#define DIVIDE 32          ///Divide valor em uma região da memória pelo acumulador
#define MULTIPLY 33        ///Multiplica valor em uma região da memória pelo acumulador
//Operações de transferência de controle
#define BRANCH 40          ///Desvio incondicional
#define BRANCHNEG 41       ///Desvio condicional acumulador negativo
#define BRANCHZERO 42      ///Desvio condicional acumulador zero
#define HALT 43            ///Termina O programa
#define REM 0
#define INPUT 1
#define LET 2
#define PRINT 3
#define GOTO 4
#define IF 5
#define END 6
#define GOSUB 7
#define RETURN 8
#define FOR 9
#define NEXT 10
#define VAR 'V'
#define LINE 'L'
#define CONST 'C'
#define STRING 'S'
#define ARRAY 'A'
#define MAIORQ 0
#define MENORQ 1
#define IGUAL 2
#define MAIORIGUAL 3
#define MENORIGUAL 4
#define DIFERENTE 5
struct CharStackNode {
   char data;
   struct CharStackNode* proxPtr;
};
struct  DoubleStackNode {
   char data_char;
   int memory_add_data;
   struct DoubleStackNode* proxPtr;
};
typedef struct CharStackNode CHARNODESTACK;
typedef CHARNODESTACK* CHARNODESTACKPTR;
typedef struct DoubleStackNode PILHA_MEMORIA;
typedef PILHA_MEMORIA* PILHA_MEMORIA_PTR;
struct entradaTabela {
   ///Representação de uma variável, numero de linha ou de constante
   int simbolo;
   ///Simbolo 'C', 'L' linha ou 'V' para variável
   char tipo;
   ///Local da memória
   int local;
} tabelaSimbolos[TAM];
void **to_free=NULL;
size_t fsize=0;
void push_to_free(void* tofree){
   ++fsize;
   to_free=realloc(to_free,sizeof(void*)*fsize);
   to_free[fsize-1]=tofree;
}
const char const *comandosLSIMPLES[11]= {"rem","input","let","print","goto","if", "end","gosub","return","for","next"};
int MEMORY[TAM]= {0},flag[TAM],acumulador=0,instrucao,InstrucaoATUAL=0,linhaPassada=0,contadorLMS=0,endereco_var=TAM-1,MEMORIA_ATUAL=0;
void registra_log(void) {
   int mem;
   FILE *logPtr;
   logPtr=fopen("log.lms.txt","a");
   if(logPtr!=NULL) {
      fprintf(logPtr,"\n\
           \rAcumulador:         \t%10d\n\
           \rInstrução atual:    \t%10d\n\
           \rCódigo de Instrução:\t%10d\n\
           \rRegião da memória:  \t%10d\n\n\
           \rMEMÓRIA:\n\
           \r\t",acumulador,InstrucaoATUAL,instrucao,PegaMemoria(instrucao));
      for(mem=0; mem<10; mem++) {
         fprintf(logPtr,"%6d ",mem);
      }
      for(mem=0; mem<TAM; mem++) {
         (mem%10+1!=1?fprintf(logPtr," "):fprintf(logPtr,"\n%3d\t ",mem/10*10));
         fprintf(logPtr,"%+06d ",MEMORY[mem]);
      }
      fprintf(logPtr,"\n");
      fclose(logPtr);
   } else {
      printf("Erro ao criar arquivo de log!\n\n");
   }
}
void printaMemoria(void) {
   int mem;
   printf("\n\
           \rAcumulador:         \t%10d\n\
           \rInstrução atual:    \t%10d\n\
           \rCódigo de Instrução:\t%10d\n\
           \rRegião da memoória: \t%10d\n\n\
           \rMEMÓRIA:\n\
           \r",acumulador,InstrucaoATUAL,Get_Instruction_mem(instrucao),PegaMemoria(instrucao));
   for(mem=0; mem<10; mem++) {
      printf("%*d",int_tam(DESLOCAMEM)+3,mem);
   }
   printf("\n");
   for(mem=0; mem<TAM; mem++) {
      (mem%10+1!=1?printf(" "):printf("\n%*d ",int_tam(TAM),mem/10*10));
      printf("%+0*d",int_tam(DESLOCAMEM)+2,MEMORY[mem]);
   }
   printf("\n\n\n");
   registra_log();
}
void reiniciaMemoria(void) {
   for(int a=0; a<TAM; a++) {
      MEMORY[a]=0;
      flag[a]=-1;
   }
}
int int_tam(int n) {
   int t=0,s=n;
   while(s)
      t++,s/=10;
   return n?t:1;
}
void colocar_fila(PILHA_MEMORIA_PTR *headPtr,PILHA_MEMORIA_PTR *tailPtr,char cValor,int dValor) {
   PILHA_MEMORIA_PTR newPtr;
   newPtr=malloc(sizeof(PILHA_MEMORIA));
   if(newPtr!=NULL) {
      newPtr->data_char=cValor;
      newPtr->memory_add_data=dValor;
      if((*headPtr)==NULL) {
         *headPtr=newPtr;
      } else {
         (*tailPtr)->proxPtr=newPtr;
      }
      *tailPtr=newPtr;
   } else {
      printf("Valores não inseridos.\n Não existe memória disponível!\n");
   }
}
void print_memory_Stack(PILHA_MEMORIA_PTR StackOperadores) {
   PILHA_MEMORIA_PTR tempPtr=StackOperadores;
   if(tempPtr!=NULL) {
      printf("\nA lista é: ");
      while(tempPtr!=NULL) {
         if(tempPtr->data_char!='\0') {
            printf("%c ",tempPtr->data_char);
         } else {
            printf("%2d ",tempPtr->memory_add_data);
         }
         tempPtr=tempPtr->proxPtr;
      }
   } else {
      printf("A fila está vazia!\n");
   }
}
void Push_memory_Stack(PILHA_MEMORIA_PTR* StackOperadores,int Dop) {
   PILHA_MEMORIA_PTR NewStackNodePtr=malloc(sizeof(PILHA_MEMORIA));
   if(NewStackNodePtr!=NULL) {
      NewStackNodePtr->memory_add_data=Dop;
      NewStackNodePtr->data_char='\0';
      NewStackNodePtr->proxPtr=*StackOperadores;
      *StackOperadores=NewStackNodePtr;
   } else {
      printf("Alocação mal sucedida, sem memória!\n");
   }
}
void compila_simples_to_simpltron(void) {
   FILE* compilado=fopen("Compilado_simples.txt","w");
   if(compilado!=NULL) {
      for(int mem=0; mem<TAM; mem++) {
         fprintf(compilado,"%04d\n",MEMORY[mem]);
      }
      fclose(compilado);
      printf("\n");
   } else {
      printf("Erro ao compilar!\n");
   }
}
void EscreveNaMemoria(const int valor) {
   MEMORY[MEMORIA_ATUAL++]=valor;
}
int Get_Instruction_mem(int n) {
   int a=0,temp=n;
   while(temp>99) {
      temp/=10;
      a++;
   }
   return n/(int)pow(10,a);
}
int PegaMemoria (int n) {
   int a = 0, temp = n, retorno = 0;
   if (temp < TAM) {
      return n;
   } else {
      while (temp > 99) {
         retorno +=( (temp % 10) * (int)pow(10,a));
         temp /= 10;
         a++;
      }
   }
   return retorno;
}
int float_to_int(float nf) {
   int n1=(int)nf,n2;
   char temp[20],*p;
   sprintf(temp,"%.2f",(float)(nf-(float)n1));
   p=&temp[2];
   n2=atoi(p);
   return n1*PRECISAO+n2;
}
void executar(void) {
   int indice=0,controle=0,localMemoria;
   float entValor;
   int exec_instrucao=Get_Instruction_mem(MEMORY[indice]);
   FILE *logPtr;
   logPtr=fopen("log.lms.txt","a");
   for(indice=0; indice<TAM&&controle==0; indice++) {
      exec_instrucao=Get_Instruction_mem(MEMORY[indice]);
      localMemoria=PegaMemoria(MEMORY[indice]);
      InstrucaoATUAL=indice;
      instrucao=MEMORY[indice];
      printaMemoria();
      switch(exec_instrucao) {
      case READ: {
         printf("\nDigige um valor para escrever no endereço %2d: ",localMemoria);
         fprintf(logPtr,"\nDigige um valor para escrever no endereço %2d: ",localMemoria);
         scanf("%f",&entValor);
         fprintf(logPtr,"%f",entValor);
         MEMORY[localMemoria]=float_to_int(entValor);
         break;
      }
      case WRITE: {
         printf("\n---------------------------------\n\rValor da memória %2d = %5d.%02d\n---------------------------------\n",localMemoria,MEMORY[localMemoria]/PRECISAO,MEMORY[localMemoria]%PRECISAO);
         fprintf(logPtr,"\n---------------------------------\n\rValor da memória %2d = %+05d\n---------------------------------\n",localMemoria,MEMORY[localMemoria]);
         break;
      }
      case STR_PRINT: {
         int string_s=MEMORY[localMemoria]/1000;
         printf("---------------------------------");
         printf("\nString : ");
         for(int a=0; a<string_s; a++) {
            printf("%c",MEMORY[localMemoria+a]%1000);
         }
         printf("\n---------------------------------\n\n");
         break;
      }
      case LOAD: {
         acumulador=MEMORY[localMemoria];
         break;
      }
      case STORE: {
         MEMORY[localMemoria]=acumulador;
         break;
      }
      case ADD: {
         acumulador+=MEMORY[localMemoria];
         break;
      }
      case SUBTRACT: {
         acumulador-=MEMORY[localMemoria];
         break;
      }
      case DIVIDE: {
         acumulador=float_to_int((((float)acumulador)/PRECISAO)/((float)((float)MEMORY[localMemoria])/PRECISAO));
         break;
      }
      case MULTIPLY: {
         acumulador=float_to_int((((float)acumulador)/PRECISAO)*((float)((float)MEMORY[localMemoria])/PRECISAO));
         break;
      }
      case BRANCH: {
         indice = localMemoria-1;
         break;
      }
      case BRANCHNEG: {
         if(acumulador<0) {
            indice = localMemoria-1;
         }
         break;
      }
      case BRANCHZERO: {
         if(acumulador==0) {
            indice = localMemoria-1;
         }
         break;
      }
      case HALT: {
         printf("\n\nEncerrando o programa.....\n");
         fprintf(logPtr,"\n\nEncerrando o programa.....\n");
         controle++;
         break;
      }
      }
   }
}
void leitura_pelo_arquivo(void) {
   FILE *comSptr=fopen("Compilado_simples.txt","r");
   int NumLinha;
   char *operacao=NULL;
   size_t len = 0;
   if(comSptr!=NULL) {
      printf("\t  Bem vindo ao Simpletron!\n\
              \r  Por favor aguarde enquanto as instruções estão sendo lidas....\n");
      while ((getline(&operacao, &len, comSptr)) != -1) {
         instrucao=atoi(operacao);
         EscreveNaMemoria(instrucao);
      }
      free(operacao);
      fclose(comSptr);
      executar();
   } else {
      printf("As instruções não puderam ser lidas!\n");
   }
}
void Push_char_Stack(CHARNODESTACKPTR* StackOperadores,char Dop) {
   CHARNODESTACKPTR NewStackNodePtr=malloc(sizeof(CHARNODESTACK));
   if(NewStackNodePtr!=NULL) {
      NewStackNodePtr->data=Dop;
      NewStackNodePtr->proxPtr=*StackOperadores;
      *StackOperadores=NewStackNodePtr;
   } else {
      printf("Alocação mal sucedida, sem memória!\n");
   }
}
char pop_char_Stack(CHARNODESTACKPTR* StackOperadores) {       ///Pilha de operadores | retira
   CHARNODESTACKPTR tempPtr=*StackOperadores;
   char Dop=(*StackOperadores)->data;
   *StackOperadores=(*StackOperadores)->proxPtr;
   free(tempPtr);
   return Dop;
}
int pop_memory_Stack(PILHA_MEMORIA_PTR* StackOperadores) {  ///Pilha de operandos | retira
   PILHA_MEMORIA_PTR tempPtr=*StackOperadores;
   int Dop=(*StackOperadores)->memory_add_data;
   *StackOperadores=(*StackOperadores)->proxPtr;
   free(tempPtr);
   return Dop;
}
char top_char_Stack(CHARNODESTACKPTR topoPtr) {
   return topoPtr->data;
}
int seOperador(char c) { ///Verifica se é um operador
   char operadores[]= {"+-*/%^"};
   for(int a=0; operadores[a]!='\0'; a++)
      if(c==operadores[a])
         return 1;
   return 0;
}
void print_char_Stack(CHARNODESTACKPTR StackOperadores) {
   CHARNODESTACKPTR tempPtr=StackOperadores;
   if(tempPtr!=NULL) {
      printf("A pilha é: ");
      while(tempPtr!=NULL) {
         printf("%c--> ",tempPtr->data);
         tempPtr=tempPtr->proxPtr;
      }
      printf("NULL\n");
   } else {
      printf("A pilha está vazia!\n");
   }
}
int newPrec(char operador) {
   int vetor[255]= {0};
   vetor['(']=1,vetor['+']=2,vetor['-']=2,vetor['*']=3,vetor['/']=3,vetor['%']=4,vetor['^']=5;
   return vetor[operador];
}
int pega_endereco(char tipo,int simbolo) {
   for(int a=0; a<TAM; a++) {
      if(tabelaSimbolos[a].simbolo==simbolo&&tabelaSimbolos[a].tipo==tipo) {
         return tabelaSimbolos[a].local;
      }
   }
   return -1;
}
PILHA_MEMORIA_PTR PostFix_Converter(char infix[]) {
   CHARNODESTACKPTR pilhaExpressao=NULL;
   Push_char_Stack(&pilhaExpressao,'(');
   char *infixPtr=infix;
   strncat(infix,")",1);
   int teste;
   PILHA_MEMORIA_PTR head=NULL,tail=NULL;
   while(*infixPtr!='\0') {
      if(isdigit(*infixPtr)||*infixPtr=='.') {
         int valor=pega_endereco(CONST,float_to_int(strtod(infixPtr,&infixPtr)));
         if(valor!=-1) {
            colocar_fila(&head,&tail,'\0',valor);
         }
      } else if(*infixPtr=='(') {
         Push_char_Stack(&pilhaExpressao,*infixPtr);
         *infixPtr++;
      } else if(seOperador(*infixPtr)) {
         while(newPrec(pilhaExpressao->data)>= newPrec(*infixPtr)) {
            colocar_fila(&head,&tail,pilhaExpressao->data,0);
            pop_char_Stack(&pilhaExpressao);
         }
         Push_char_Stack(&pilhaExpressao,*infixPtr);
         *infixPtr++;
      } else if(*infixPtr==')') {
         while(!charEstaVazia(pilhaExpressao)&&top_char_Stack(pilhaExpressao)!='(') {
            colocar_fila(&head,&tail,pilhaExpressao->data,0);
            pop_char_Stack(&pilhaExpressao);
         }
         if(top_char_Stack(pilhaExpressao)=='(') {
            pop_char_Stack(&pilhaExpressao);
         }
         *infixPtr++;
      } else if(*infixPtr==' ') {
         *infixPtr++;
      } else if(isalpha(*infixPtr)) {
         if(pega_endereco(VAR,*infixPtr)!=-1) {
            colocar_fila(&head,&tail,'\0',pega_endereco(VAR,*infixPtr));
         } else if (pega_endereco(ARRAY,*infixPtr)!=-1) {
            int n_endereco=pega_endereco(ARRAY,*infixPtr);
            infixPtr=strpbrk(infixPtr,"[")+1;
            printf("oi'%s'\n",infixPtr);
            if(isdigit(*infixPtr)) {
               int n_desloc=(int)strtod(infixPtr,&infixPtr);
               colocar_fila(&head,&tail,'\0',n_endereco+n_desloc);
            } else {
               printf("Erro: tentativa de usar variável como subscrito de array!\nNão consegue né!\n");
               assert(1);
            }
         }
         *infixPtr++;
      } else {
         *infixPtr++;
      }
   }
   return head;
}
void adiciona_Tabela(int sim,char t, int l) {
   tabelaSimbolos[linhaPassada].simbolo=sim;
   tabelaSimbolos[linhaPassada].tipo=t;
   tabelaSimbolos[linhaPassada].local=l;
}
int calcula(int operando1,int operando2, char operador) {
   int resultado=endereco_var;
   switch(operador) {
   case '^': {
      if(pega_endereco(CONST,1)==-1) {
         endereco_var--;
         adiciona_Tabela(1,CONST,endereco_var);
         MEMORY[endereco_var--]=100;
         linhaPassada++;
      }
      int tmp=endereco_var--;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+pega_endereco(CONST,1);
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+resultado;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+tmp;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+operando2;
      MEMORY[contadorLMS++]=BRANCHZERO*DESLOCAMEM+contadorLMS+7;
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+pega_endereco(CONST,1);
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+operando2;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+resultado;
      MEMORY[contadorLMS++]=MULTIPLY*DESLOCAMEM+tmp;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+resultado;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM+contadorLMS-7;
      break;
   }
   case '+': {
      MEMORY[contadorLMS++]=ADD*DESLOCAMEM+operando2;
      break;
   }
   case '-': {
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+operando2;
      break;
   }
   case '*': {
      MEMORY[contadorLMS++]=MULTIPLY*DESLOCAMEM+operando2;
      break;
   }
   case '/': {
      MEMORY[contadorLMS++]=DIVIDE*DESLOCAMEM+operando2;
      break;
   }
   case '%': {
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+operando2;
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+resultado;
      MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM+contadorLMS+4;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+resultado;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM+contadorLMS+5;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+operando2;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+operando1;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM+contadorLMS-10;
      break;
   }
   default: {
      break;
   }
   }
   return resultado;
}
void printa_tabela_simbolos(void) {
   printf("\nTabela de simbolos:\nSimbolo\t\tTipo\tLocal\n");
   for(int a=0; a<TAM; a++) {
      if(isalpha(tabelaSimbolos[a].tipo)) {
         if(tabelaSimbolos[a].tipo==LINE) {
            printf("%d",tabelaSimbolos[a].simbolo);
         } else if(tabelaSimbolos[a].tipo==VAR||tabelaSimbolos[a].tipo==STRING||tabelaSimbolos[a].tipo==ARRAY) {
            printf("%c",tabelaSimbolos[a].simbolo);
         } else if (tabelaSimbolos[a].tipo==CONST) {
            printf("%d",tabelaSimbolos[a].simbolo);
         }
         printf("\t\t%c\t%6d\n",tabelaSimbolos[a].tipo,tabelaSimbolos[a].local);
      }
   }
}
int CalculaExpressaoPosfixada(char *stringExpr) {
   PILHA_MEMORIA_PTR expressaoPtr=PostFix_Converter(stringExpr),minhaStack=NULL;
   int num1=0,num2=0,tmp=0,test=0;
   char operador;
   if(expressaoPtr!=NULL) {
      if(expressaoPtr->proxPtr==NULL) {
         int ret=expressaoPtr->memory_add_data;
         while(expressaoPtr)
            pop_memory_Stack(&expressaoPtr);
         return ret;
      } else {
         while(expressaoPtr!=NULL) {
            if(!seOperador(expressaoPtr->data_char)) {
               Push_memory_Stack(&minhaStack,pop_memory_Stack(&expressaoPtr));
            } else {
               operador=expressaoPtr->data_char;
               num2=pop_memory_Stack(&minhaStack);
               if(test==0) {
                  num1=pop_memory_Stack(&minhaStack);
                  MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+num1;
               }
               tmp=calcula(num1,num2,operador);
               pop_memory_Stack(&expressaoPtr);
            }
         }
         MEMORY[contadorLMS++]=STORE*DESLOCAMEM+num1;
      }
   } else {
      return -1;
   }
   return tmp;
}
void tirarFila(PILHA_MEMORIA_PTR* headPtr, PILHA_MEMORIA_PTR *tailPtr) {
   PILHA_MEMORIA_PTR tempPtr;
   tempPtr=*headPtr;
   *headPtr=(*headPtr)->proxPtr;
   if(*headPtr==NULL) {
      *tailPtr=NULL;
   }
   free(tempPtr);
}
int charEstaVazia(CHARNODESTACKPTR topoPtr) {
   return topoPtr==NULL?1:0;
}
int PegaSimbolo(char *operacao) {
   char novo[30];
   int n=0,n1=0;
   while(!isalpha(operacao[n])) {
      n++;
   }
   while(isalpha(operacao[n])) {
      novo[n1]=operacao[n],n++,n1++;
   }
   novo[n1]='\0';
   for(int a=0; comandosLSIMPLES[a]!=NULL; a++)
      if(strncmp(comandosLSIMPLES[a],novo,strlen(comandosLSIMPLES[a]))==0)
         return a;
   return -1;
}
int opRelacionais(char *opr) {
   char *operadores[10]= {">","<","==",">=","<=","!="};
   char *tmp=strpbrk(opr,"<>=!");
   for(int a=0; a<6; a++) {
      if(strncmp(operadores[a],tmp,strlen(operadores[a]))==0) {
         return a;
      }
   }
   return -1;
}
int adiciona_na_tabela(char* exPtr) {
   char *expressao=exPtr;
   while(*expressao!='\0') {
      if(isalpha(*expressao)) {
         if(pega_endereco(VAR,*expressao)==-1&&pega_endereco(ARRAY,*expressao)==-1&&pega_endereco(STRING,*expressao)==-1) {
            printf("Referencia não definida para %c!\n",*expressao);
            return 0;
         } else if(pega_endereco(ARRAY,*expressao)!=-1) {
            expressao=strpbrk(expressao,"[")+1;
            int valor=(int)strtod(expressao,&expressao);
            if(pega_endereco(CONST,valor)==-1) {
               adiciona_Tabela(valor,CONST,endereco_var);
               MEMORY[endereco_var]=valor;
               endereco_var--;
               linhaPassada++;
            }
            expressao=strpbrk(expressao,"]");
         }
         *expressao++;
      } else if (isdigit(*expressao)) {
         float teste=strtod(expressao,&expressao);
         int  valor=float_to_int(teste);
         if(pega_endereco(CONST,valor)==-1) {
            adiciona_Tabela(valor,CONST,endereco_var);
            MEMORY[endereco_var]=valor;
            endereco_var--;
            linhaPassada++;
         }
      } else {
         *expressao++;
      }
   }
   return 1;
}
char* tira_espacos(char* palavra) {
   char *resultado=malloc(sizeof(char)*strlen(palavra));
   push_to_free(resultado);
   int a=0,b=0;
   for(a=0; a<strlen(palavra); a++) {
      if(!isspace(palavra[a])) {
         resultado[b++]=palavra[a];
      }
   }
   resultado[b]='\0';
   return resultado;
}
int lmsSimbolos(char *opr) {
   int sComando=PegaSimbolo(opr);
   int linha=(int)strtod(opr,&opr);
   char *restante=malloc(strlen(strstr(opr,comandosLSIMPLES[sComando]))*sizeof(char));
   push_to_free(restante);
   strcpy(restante,strstr(opr,comandosLSIMPLES[sComando])+strlen(comandosLSIMPLES[sComando]));
   restante[strlen(restante)-1]='\0';
   while(isspace(*restante)) {
      *restante++;
   }
   switch(sComando) {
   case REM: {
      char* tmp=strstr(opr,comandosLSIMPLES[sComando]);
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      break;
   }
   case INPUT: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      char *temPtr=tira_espacos(restante);
      for(int a=0; temPtr[a]!=NULL; a++) {
         if(a%2==0) {
            if(isalpha(temPtr[a])) {
               if(pega_endereco(VAR,temPtr[a])==-1) {
                  adiciona_Tabela(temPtr[a],VAR,endereco_var);
                  MEMORY[contadorLMS++]=READ*DESLOCAMEM+endereco_var--;
                  linhaPassada++;
               }
            } else {
               printf("%d:erro de sintaxe: inválida: %c\n",linha,temPtr[a]);
            }
         } else {
            if(temPtr[a]!=',') {

               return INPUT*100+2;
            }
         }
      }
      break;
   }
   case LET: {
      restante=tira_espacos(restante);
      adiciona_Tabela(linha,LINE,contadorLMS),linhaPassada++;
      char *teste=malloc(sizeof(char)*strlen(restante));
      push_to_free(teste);
      strcpy(teste,restante);
      if(strpbrk(teste,"=")!=NULL) {
         if(strpbrk(strtok(teste,"="),"[")!=NULL) {
            if(strpbrk(strpbrk(strtok(teste,"="),"[")+1,"]")!=NULL) {
               int endereco_array=endereco_var,definidos;
               char variavel_array=*teste;
               teste=strpbrk(strtok(teste,"="),"[")+1;
               if(!isdigit(*teste)) {
                  printf("%d:Erro: declaração de array de tamanho não especificado!\n",linha);
                  return LET*100+6;
               } else {
                  definidos=atoi(teste);
                  if(definidos<=0&&pega_endereco(ARRAY,variavel_array)==-1) {
                     printf("%d:Tentativa de inicialização de array de tamanho <=0!\n",linha);
                     return LET*100+7;
                  }
               }
               strcpy(teste,restante);
               teste=strpbrk(teste,"=")+1;
               if(pega_endereco(ARRAY,variavel_array)==-1) {
                  int temp[TAM]= {0},total=0;
                  if(!isdigit(teste[strlen(teste)-1])) {
                     if(!strlen(teste)) {
                        adiciona_Tabela(variavel_array,ARRAY,endereco_array-definidos+1),linhaPassada++;
                        for(int a=0; a<definidos; a++) {
                           MEMORY[endereco_array-definidos+1+a]=0;
                           endereco_var--;
                        }
                     } else {
                        printf("%d:Erro: presença de delimitador ',' com a ausência de constante numérica!\n",linha);


                        return LET*100+8;
                     }

                  } else {
                     while(*teste!=NULL) {
                        if(isdigit(*teste)) {
                           temp[total++]=float_to_int(strtod(teste,&teste));
                        } else if(*teste==',') {
                           teste++;
                        } else {
                           printf("%d:Erro: divisor na declaração de array:'%c' não reconhecido!\n",linha,*teste);


                           return LET*100+3;
                        }
                     }

                     if(definidos<total) {
                        printf("%d:Erro: quantidades de elementos declarados é maior que o especificado pelo inicializador do array.\nDefinidos:%d\nInicializado:%d\n",linha,definidos,total);

                        return LET*100+9;
                     } else {
                        adiciona_Tabela(variavel_array,ARRAY,endereco_array-definidos+1),linhaPassada++;
                        printf("definindo: ");
                        for(int a=0; a<definidos; a++) {
                           if(a<=total) {
                              MEMORY[endereco_array-definidos+1+a]=temp[a];
                           } else {
                              MEMORY[endereco_array-definidos+1+a]=0;
                           }
                           printf("%d ",temp[a]);
                           endereco_var--;
                        }
                     }
                  }
               } else {
                  int v_ende=pega_endereco(ARRAY,variavel_array);
                  strcpy(teste,restante);
                  teste=strpbrk(teste,"[")+1;
                  int n_desl=(int)strtod(teste,&teste);
                  teste=strpbrk(teste,"=")+1;
                  adiciona_na_tabela(teste);
                  int n_valor=CalculaExpressaoPosfixada(teste);
               }
            } else {
               printf("%d:Faltando ']'\n",linha);

               return LET*100+5;
            }
         } else {
            char esq_var=*restante;
            restante=strpbrk(restante,"=")+1;
            if(strpbrk(restante,"\"")==NULL&&strpbrk(restante,",")==NULL) {
               if(pega_endereco(VAR,esq_var)==-1) {
                  if(pega_endereco(STRING,esq_var)==-1) {
                     int novo=endereco_var--;
                     adiciona_Tabela(esq_var,VAR,novo);
                     linhaPassada++;
                     if(!adiciona_na_tabela(tira_espacos(restante))) {

                        return LET*100+10;
                     }
                     int temp=CalculaExpressaoPosfixada(restante);
                  } else {
                     printf("%d: Erro reatribuição de tipo de variavel para %c\n",linha,esq_var);

                     return LET*100+2;
                  }
               } else {
                  int novo=pega_endereco(VAR,esq_var);
                  if(!adiciona_na_tabela(tira_espacos(restante))) {

                     return LET*100+10;
                  }
                  int temp=CalculaExpressaoPosfixada(restante);
               }
            } else if(strpbrk(restante,",")!=NULL) {
               printf("%d:Erro de atribuição? :%s\n",linha,restante);

               return LET*100+4;
            } else {
               char *tmp=strtok(strpbrk(restante,"\"")+1,"\"");
               int n=strlen(tmp)-1;
               if(pega_endereco(STRING,esq_var)==-1) {
                  if(pega_endereco(VAR,esq_var)==-1) {
                     int novo=endereco_var-n;
                     adiciona_Tabela(esq_var,STRING,novo),linhaPassada++;
                     while(n) {
                        MEMORY[endereco_var--]=tmp[n--];
                     }
                     MEMORY[endereco_var--]=strlen(tmp)*1000+tmp[n--];
                  }
               } else {
                  printf("%d: Erro reatribuição de tipo de variável para %c\n",linha,esq_var);

                  return LET*100+1;
               }
            }
         }
      } else {
         printf("%d:O comando LET necessita ao menos de um inicializador '='\n",linha);

         return LET*100+5;
      }
      break;
   }
   case PRINT: {
      adiciona_Tabela(linha,LINE,contadorLMS),linhaPassada++;
      char *temPtr=malloc(sizeof(char)*strlen(restante));
      push_to_free(temPtr);
      strcpy(temPtr,tira_espacos(restante));
      for(int a=0; temPtr[a]!='\0'; a++) {
         if(isalpha(temPtr[a])) {
            if(pega_endereco(VAR,temPtr[a])!=-1||pega_endereco(STRING,temPtr[a])!=-1||pega_endereco(ARRAY,temPtr[a])!=-1) {
               if(pega_endereco(VAR,temPtr[a])!=-1) {
                  MEMORY[contadorLMS++]=WRITE*DESLOCAMEM+pega_endereco(VAR,temPtr[a]);
               } else if(pega_endereco(STRING,temPtr[a])!=-1) {
                  MEMORY[contadorLMS++]=STR_PRINT*DESLOCAMEM+pega_endereco(STRING,temPtr[a]);
               } else if(pega_endereco(ARRAY,temPtr[a])!=-1) {
                  int enderr=pega_endereco(ARRAY,temPtr[a]),valor=atoi(strpbrk(temPtr,"[")+1);
                  temPtr=strpbrk(temPtr,"]");
                  if((enderr+valor)>TAM-1||enderr+valor<=0) {
                     printf("%d:Erro ao tentar acessar local da memória inexistente!\n",linha);

                     return PRINT*100+1;
                  }
                  MEMORY[contadorLMS++]=WRITE*DESLOCAMEM+enderr+valor;
               }
            } else {
               printf("Erro referência não definida Para: %c\n",temPtr[a]);

               return PRINT*100+2;
            }
         } else if(temPtr[a]!=NULL&&temPtr[a]!=',') {
            printf("erro de sintaxe na linha:%d: '%c'\n",linha,*temPtr);

            return PRINT*100+4;
         }
      }
      break;
   }
   case GOTO: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      int desvio=atoi(strstr(strstr(opr,comandosLSIMPLES[sComando])," "));
      flag[contadorLMS]=desvio;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM;
      break;
   }
   case IF: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      if(pega_endereco(VAR,*restante)==-1) {
         adiciona_Tabela(*restante,VAR,endereco_var--);
         linhaPassada++;
      }
      char *CexpressaoEsq=malloc(sizeof(char)*(strlen(restante)-strlen(strpbrk(restante,"!<>="))));
      push_to_free(CexpressaoEsq);
      strcpy(CexpressaoEsq,restante);
      CexpressaoEsq[strlen(restante)-strlen(strpbrk(restante,"!<>="))]='\0';
      int relacional=opRelacionais(strpbrk(restante,"!<>="));
      char *CexpressaoDir=malloc(sizeof(char)*strlen(restante)-strlen(CexpressaoEsq));
      push_to_free(CexpressaoDir);
      char *tempO=strpbrk(restante+strlen(CexpressaoEsq),"!<>=");
      while(strpbrk(tempO,"!<>=")!=NULL) {
         *tempO++;
      }
      strcpy(CexpressaoDir,tempO);
      CexpressaoDir[strlen(CexpressaoDir)-strlen(strstr(CexpressaoDir,"goto"))]='\0';
      if( !adiciona_na_tabela(tira_espacos(CexpressaoEsq)) || !adiciona_na_tabela(tira_espacos(CexpressaoDir)) ) {
         return LET*100+10;
      }
      int expressaoEsq=CalculaExpressaoPosfixada(CexpressaoEsq);
      int expressaoDir=CalculaExpressaoPosfixada(CexpressaoDir);
      int tmpLocal=atoi(strpbrk(strstr(restante,"goto"),"1234567890"));
      switch(relacional) {
      case DIFERENTE: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoEsq;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoDir;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoDir;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoEsq;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         break;
      }
      case MENORQ: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoEsq;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoDir;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         break;
      }
      case MENORIGUAL: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoEsq;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoDir;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         MEMORY[contadorLMS++]=BRANCHZERO*DESLOCAMEM;
         break;
      }
      case MAIORQ: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoDir;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoEsq;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         break;
      }
      case MAIORIGUAL: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoDir;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoEsq;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHNEG*DESLOCAMEM;
         MEMORY[contadorLMS++]=BRANCHZERO*DESLOCAMEM;
         break;
      }
      case IGUAL: {
         MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+expressaoEsq;
         MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+expressaoDir;
         flag[contadorLMS]=tmpLocal;
         MEMORY[contadorLMS++]=BRANCHZERO*DESLOCAMEM;
         break;
      }
      }
      break;
   }
   case END: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      MEMORY[contadorLMS++]=HALT*DESLOCAMEM;
      linhaPassada++;
      break;
   }
   case GOSUB: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      int desvio=atoi(strstr(strstr(opr,comandosLSIMPLES[sComando])," "));
      flag[contadorLMS]=desvio;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM;
      break;
   }
   case RETURN: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      int desvio=atoi(strstr(strstr(opr,comandosLSIMPLES[sComando])," "));
      flag[contadorLMS]=desvio;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM;
      break;
   }
   case FOR: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      if(pega_endereco(VAR,*restante)==-1) {
         adiciona_Tabela(*restante,VAR,endereco_var--);
         linhaPassada++;
      }
      int desti=pega_endereco(VAR,*restante);
      restante=strstr(restante,"=");
      char *temporario=malloc(sizeof(char)*strlen(restante));
      push_to_free(temporario);
      strcpy(temporario,restante);
      temporario++;
      int memmoria=float_to_int(strtod(temporario,&temporario));
      if(pega_endereco(CONST,memmoria)==-1) {
         adiciona_Tabela(memmoria,CONST,endereco_var);
         MEMORY[endereco_var--]=memmoria;
         linhaPassada++;
      }
      memmoria=pega_endereco(CONST,memmoria);
      temporario=strstr(restante,"to")+2;
      int numFinal=float_to_int(strtod(temporario,&temporario));
      if(pega_endereco(CONST,numFinal)==-1) {
         adiciona_Tabela(numFinal,CONST,endereco_var);
         MEMORY[endereco_var--]=numFinal;
         linhaPassada++;
      }
      int step,desvio;
      if(strstr(temporario,"step")!=NULL) {///Step >1
         temporario=strstr(temporario,"step")+4;
         step=float_to_int(strtod(temporario,&temporario));
         temporario=strstr(temporario,"goto")+4;
         desvio=(int)strtod(temporario,&temporario);
      } else {
         step=1*PRECISAO;
         temporario=strstr(temporario,"goto")+4;
         desvio=(int)strtod(temporario,&temporario);
      }
      if(pega_endereco(CONST,step)==-1) {
         adiciona_Tabela(step,CONST,endereco_var);
         MEMORY[endereco_var--]=step;
         linhaPassada++;
      }
      step=pega_endereco(CONST,step);
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+desti;
      MEMORY[contadorLMS++]=ADD*DESLOCAMEM+memmoria;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+desti;
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+pega_endereco(CONST,numFinal);
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM+contadorLMS+3;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+desti;
      MEMORY[contadorLMS++]=SUBTRACT*DESLOCAMEM+pega_endereco(CONST,numFinal);
      flag[contadorLMS]=desvio;
      MEMORY[contadorLMS++]=BRANCHZERO*DESLOCAMEM;
      MEMORY[contadorLMS++]=LOAD*DESLOCAMEM+desti;
      MEMORY[contadorLMS++]=ADD*DESLOCAMEM+step;
      MEMORY[contadorLMS++]=STORE*DESLOCAMEM+desti;
      break;
   }
   case NEXT: {
      adiciona_Tabela(linha,LINE,contadorLMS);
      linhaPassada++;
      int desvio=(int)strtod(restante,&restante);
      flag[contadorLMS]=desvio;
      MEMORY[contadorLMS++]=BRANCH*DESLOCAMEM+5;
      break;
   }
   }
   return 0;
}
void pass_2(void) {
   printf("Antes da segunda passada: \n");
   printaMemoria();
   for(int a=0; a<TAM; a++)
      if(flag[a]!=-1)
         for(int b=0; b<TAM; b++)
            if(tabelaSimbolos[b].simbolo==flag[a])
               MEMORY[a]+=tabelaSimbolos[b].local;
   printf("\nDepois da segunda passada:\n");
   printaMemoria();
}
void LMS(char* filename) {
   reiniciaMemoria();
   FILE *ArqLms=fopen(filename,"r");
   int NumLinha,status=0;;
   char *operacao=NULL,*temp;
   size_t len = 0;
   if(ArqLms!=NULL) {
      while ((getline(&operacao, &len, ArqLms)) != -1&&!status) {
         temp=operacao;
         while(*temp!='\0') {
            *temp=tolower(*temp);
            *temp++;
         }
         status=lmsSimbolos(operacao);
         if(status) {
            printf("Status:%d\n",status);
            assert(!status);
         }
      }
      fclose(ArqLms);
   } else {
      printf("Arquivo não pode ser aberto!\n");
   }
   push_to_free(operacao);
   if(!status) {
      pass_2();
      printa_tabela_simbolos();
      compila_simples_to_simpltron();
      reiniciaMemoria();
      leitura_pelo_arquivo();
   }
}
int main(int argc, char* argv[]) {
   if(argc!=2){
      printf("Quantidade inválida de parâmetros!\n");
      exit(3);
   }
   LMS(argv[1]);
   for(int i=0;i<fsize;i++)
      free(to_free[i]);
   if(fsize) free(to_free);
   return 0;
}
