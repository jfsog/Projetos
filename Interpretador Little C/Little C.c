#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
///Para o analisador
#define NUM_FUNC        100
#define NUM_VAR         100
#define NUM_VAR_GLOBAL  100
#define NUM_VAR_LOCAIS  200
#define NUM_BLOCK       100
#define ID_TAM          31
#define CHAMADAS_FUNC   31
#define NUM_PARAMS      31
#define EXPRE_TAM       10000
#define LOOP_NEST       31

void dec_global(void),pega_args(void),exec_if(void),interpreta_bloco(void);
///Funções utilizadas para o alisador recursivo
void eval_exp0(double* value),eval_exp1(double* value),
     eval_exp2(double* value),eval_exp3(double* value),
     eval_expre(double* value);
///Armazena o tipo de token ao longo do pegador de tokens
enum t_token {INVALIDO,DELIMITADOR,IDENTIFICADOR,
              NUMERO,PCHAVE,TEMP,STRING,BLOCO
             } tipo_token=INVALIDO;
///Enumeração de operadores duplos
enum op_duplos {ME=1,MEI,MA,MAI,IG,NI};
char op_relacionais[7]= {ME,MEI,MA,MAI,IG,NI,'\0'};
///Armazena a espécie de token ao longo do programa para palavras reservadas
enum tokens {ARGU,CHAR,INT,
             IF,ELSE,FOR,DO,WHILE,SWITCH,
             RETURN,EOL,FINISHED,END
            } tipo_tok;
///mensagens de erro
enum mensagens_erros {SINTAXE,PARENT_DESBA,NO_EXPRE,IGUAL_EXPERADO,
                      N_VAR,ERRO_PARAM,SEMI_ESPERADO,CHAVES_DESBALANCEADAS,
                      FUNC_UNDEF,TIPO_ESPERADO,NEST_FUNC,RET_NOCALL,
                      PARENT_ESPERADO,WHILE_EXPECTED,ASPAS_ESPERADO,
                      NOT_TEMP,MUITAS_VAR
                     };
///variável para o longjmp
jmp_buf ambiente_buffer;
///Matriz usada para armazenar variáveis globais e locais
struct table {
   char p_variavel[32];///Nome de variáveis
   double valor;///Valor da variável
   int tipo;///Tipo de variável?
} global_var_table[NUM_VAR_GLOBAL],local_var_stack[NUM_VAR_LOCAIS];

///Tabela de palavras chaves reservadas
struct comandos {
   char comando[32];
   enum tokens word_tipo_tok;
} table_reservadas[]= {
   "if",IF,
   "else",ELSE,
   "for",FOR,
   "do",DO,
   "while",WHILE,
   "char",CHAR,
   "int",INT,
   "return",RETURN,
   "end",END,
   "",END
};

///Funções definidas pelo usuário
struct func_pilha {
   char nome_funcao[32];
   char* coord;
} tabela_func_user[CHAMADAS_FUNC];
int call_stack[NUM_FUNC]= {0};

//Declaraçfo de funções
void erro_sintaxe(enum mensagens_erros tipo),buf_retorna(void);
///funções da biblioteca padrão
double call_getchar(void),call_putch(void),call_puts(void),print(void),getnum(void);

struct func_tipo_interno {
   char *nome_funcao;
   double (*p)();
} funcoes_internas[]= {
   "getchar",call_getchar,
   "putch",call_putch,
   "puts",call_puts,
   "print",print,
   "getnum",getnum,
   "",0
};

/*Índice na tabela de funções, índice na tabela de variáveis globais,
índice na tabela de variáveis locais e
índice para o topo da pilha de chamada de função*/
int func_index,gvar_index,lvartos,functos;
///Valor da expressão e de return
double avaliada_expre,retorno_valor;
///Para pegar token, armazena o token, aloca inicialmente o token aponta para o início do programa e
///expre=ponteiro utilizado ao longo do programa
char token[EXPRE_TAM],*inicio_buf,*expre;
///Define uma variável global
void define_variavel(char* nome_var,double valor) {
   global_var_table[gvar_index].valor=valor,strcpy(global_var_table[gvar_index].p_variavel,nome_var);
   gvar_index++;
}
/*função utilizada para capturar um único caractere do teclado*/
double call_getchar(void) {
   char ch;
   setbuf(stdin,NULL);
   scanf("%c",&ch);
   setbuf(stdin,NULL);
   while(*expre!=')')pega_token();
   pega_token();
   return (double)ch;
}
/*essa função é usada para imprimir um caractere
## melhorar a função para limitar a tabela ASCII entre 0 e 255
*/
double call_putch(void) {
   double value;
   eval_expre(&value);
//   printf("T:%s\n",token);
   pega_token();
   printf("%c",(int)value);
   return value;
}
/*chama puts, que imprime uma string,
que nesse caso será o 'token' capturado entre as chamadas de pega_token*/
double call_puts(void) {
   pega_token();
   if(*token!='(')erro_sintaxe(PARENT_ESPERADO);
   pega_token();
   if(tipo_token!=STRING)erro_sintaxe(ASPAS_ESPERADO);
   printf("%s\n",token);
   pega_token();
   if(*token!=')')erro_sintaxe(PARENT_ESPERADO);;
   pega_token();
   if(*token!=';')erro_sintaxe(SEMI_ESPERADO);
   buf_retorna();
   return 0;
}
/*Função utilizada para saída no console, printa strings e números*/
double print(void) {
   double i;
   pega_token();
   if(*token!='(')erro_sintaxe(PARENT_ESPERADO);
   pega_token();
   if(tipo_token==STRING) {
      printf("%s",token); ///Exibe uma string
   } else {
      buf_retorna();
      eval_expre(&i);
      printf("%.2lf ",i); ///Exibe um número em formato double
   }
   pega_token();
   if(*token!=')')erro_sintaxe(PARENT_ESPERADO);
   pega_token();
   if(*token!=';')erro_sintaxe(SEMI_ESPERADO);
   buf_retorna();
   return 0;
}
/*Lê um número do teclado*/
double getnum(void) {
   char n[100];
   setbuf(stdin,NULL);
   fgets(n,100,stdin);
   n[strlen(n)-1]='\0';
   while(*expre!=')')expre++;
   expre++;
   return (double)atof(n);
}
///Função utilizada para exibir erros de sintaxe ao longo do programa
void erro_sintaxe(enum mensagens_erros tipo) {
   char* t,*temp;
   int contador=0;
   char* erros[]= {"Erro de sintaxe","parênteses desbalanceados",
                   "falta uma expressão","esperado sinal de igual","não é uma variável",
                   "erro de parâmetro","esperado ponto e vírgula","chaves desbalanceadas",
                   "função não definida","esperado identificador de tipo",
                   "excessivas chamadas aninhadas de função","return sem chamada","esperado parênteses",
                   "esperado while","esperado fechar aspas","não é uma string","excessivas variáveis locais"
                  };
   printf("%s",erros[tipo]);
   t=inicio_buf;///Proto buff, ponteiro para o inicio do arquivo
   while(t!=expre) {
      t++;
      if(*t=='\n')contador++;
   }
   printf(" na linha %2d :",contador);
   temp=t;
   for(; t>inicio_buf&&*t!='\n'; t--) {
   }
   for(; *t!='\0'&&*t!='\n'; ) {
      printf("%c",*t++);
   }
   longjmp(ambiente_buffer,1);
}
///Verifica se é um delimitador
int se_delimitador(char t) {
   return (strchr(" !;,'+-<>*/^()%={}",t)||t==9||t=='\n'||t==0)?1:0;
}
int busca_global_var_table(char* var_name) {
   for(int a=0; a<gvar_index; a++)
      if(!strcmp(global_var_table[a].p_variavel,var_name))
         return a;
   return -1;
}
int se_variavel(char* v) {
   register int a;//Busca por variáveis locais e globais
   for(a=lvartos-1; a>=call_stack[functos-1]; a--)
      if(!strcmp(local_var_stack[a].p_variavel,v))
         return 1;
   for(a=0; a<NUM_VAR_GLOBAL; a++)
      if(!strcmp(global_var_table[a].p_variavel,v)) {
         return 1;
      }
   return 0;
}
///Procurar pela representação interna de um token na tabela de tokens
int procurar(char* busca) {
   register int a;
   char* c=busca;
   while(*c) *c++=tolower(*c);
   for(a=0; *table_reservadas[a].comando; a++)
      if(!strcmp(table_reservadas[a].comando,busca))
         return table_reservadas[a].word_tipo_tok;
   return 0;
}
///procurar biblioteca padrão e retorna o indice da função
int procurar_inter_func(char* busca) {
   for(int a=0; *funcoes_internas[a].nome_funcao; a++)
      if(!strcmp(funcoes_internas[a].nome_funcao,busca))
         return a;
   return -1;
}
char* find_func(char* nome) {
   for(int a=0; a<func_index; a++)
      if(!strcmp(nome,tabela_func_user[a].nome_funcao))return tabela_func_user[a].coord;
   return NULL;
}
void atribui_variavel(char* nome,double v_valor) {
   register int a;
   for(a=lvartos-1; a>=call_stack[functos-1]; a--) {
      if(!strcmp(local_var_stack[a].p_variavel,nome)) {
         local_var_stack[a].valor=v_valor;
         return;
      }
   }
   if(a<call_stack[functos-1])
      for(a=0; a<NUM_VAR_GLOBAL; a++)
         if(!strcmp(global_var_table[a].p_variavel,nome)) {
            global_var_table[a].valor=v_valor;
            return;
         }
   erro_sintaxe(N_VAR);
}
double encontra_var(char* s) {
   for(int a=lvartos-1; a>=call_stack[functos-1]; a--)
      if(!strcmp(local_var_stack[a].p_variavel,token))
         return local_var_stack[a].valor;
   for(int a=0; a<NUM_VAR_GLOBAL; a++)
      if(!strcmp(global_var_table[a].p_variavel,s))
         return global_var_table[a].valor;
   erro_sintaxe(N_VAR);
}
int e_espaco(char c) {
   if(c==' '||c=='\t')return 1;
   else return 0;
}
///Pega o próximo token da expressão
int pega_token(void) {
   tipo_token=INVALIDO;
   tipo_tok=0;
   char* temp=token;
   *temp='\0';
   while(e_espaco(*expre)&&*expre)++expre;///Ignora espaços vaizos

   if(*expre=='\n') {///Ignora nova linha
      ++expre;
      while(e_espaco(*expre)&&*expre)++expre;
   }

   if(*expre=='\0') {/*fim do arquivo*/
      *token='\0',tipo_tok=FINISHED;
      return(tipo_token=DELIMITADOR);
   }
   if(strchr("{}",*expre)) {///Verifica delimitador de bloco
      *temp++=*expre++,*temp='\0';
      return(tipo_token=BLOCO);
   }
   if(*expre=='/') {/// é um comentário
      if(expre[1]=='*') {
         expre+=2;
         do {
            while(*expre!='*')expre++;
            expre++;
         } while(*expre!='/');
         expre++;
      }
   }
   if(strchr("!<>=",*expre)) {///Possibilidade de ser um operador relacional
      switch(*expre) {
      case '=':
         if(expre[1]=='=') expre+=2,*temp++=IG,*temp++=IG,*temp='\0';
         break;
      case '!':
         if(expre[1]=='=') expre+=2,*temp++=NI,*temp++=NI,*temp='\0';
         break;
      case '<':
         expre[1]=='='?expre+=2,*temp++=MEI,*temp++=MEI:(expre++,*temp++=ME);
         *temp='\0';
         break;
      case '>':
         (expre[1]=='=')?expre+=2,*temp++=MAI,*temp++=MAI:(expre++,*temp++=MA);
         *temp='\0';
         break;
      }
      if(*token) return(tipo_token=DELIMITADOR);
   }
   if(strchr("+-*^/%=;(),'",*expre)) {///Delimitador
      *temp++=*expre++,*temp='\0';
      return(tipo_token=DELIMITADOR);
   } else if(*expre=='"') {///Stinrg entre aspas
      expre++;
      while(*expre!='"'&&*expre!='\n')*temp++=*expre++;
      if(*expre=='\n')erro_sintaxe(SINTAXE);
      expre++,*temp='\0';
      return(tipo_token=STRING);
   } else if(isdigit(*expre)) { ///Número
      while(!se_delimitador(*expre))*temp++=*expre++;
      *temp='\0';
      return(tipo_token=NUMERO);
   } else if(isalpha(*expre)) {
      while(!se_delimitador(*expre))*temp++=*expre++;
      tipo_token=TEMP;
   }
   *temp='\0';
   if(tipo_token==TEMP) {
      tipo_tok=procurar(token);
      tipo_token=tipo_tok?PCHAVE:IDENTIFICADOR;
   }
   return tipo_token;
}
void buf_retorna(void) {
   for(char *c=token; *c; c++)expre--;
}
void local_push(struct table t) {
   if(lvartos>NUM_VAR_LOCAIS)
      erro_sintaxe(MUITAS_VAR);
   local_var_stack[lvartos++]=t;
}
int func_pop(void) {
   functos--;
   if(functos<0)erro_sintaxe(RET_NOCALL);
   return (call_stack[functos]);
}
void func_push(int r) {
   if(functos>NUM_FUNC)
      erro_sintaxe(NEST_FUNC);
   call_stack[functos++]=r;
}
///Atribui variáveis globais e funções definidas pelo usuário
void pre_scan(void) {
   char* t=inicio_buf,temp[32];
   int n_aspas=0;
   func_index=0;
   do {
      while(n_aspas) {
         pega_token();
         if(*token=='{')n_aspas++;
         if(*token=='}')n_aspas--;
      }
      pega_token();
      if(tipo_tok==CHAR||tipo_tok==INT) {
         buf_retorna();
         dec_global();
      } else if(tipo_token==IDENTIFICADOR) {
         strcpy(temp,token);
         pega_token();
         if(*token=='(') {
            tabela_func_user[func_index].coord=expre;
            strcpy(tabela_func_user[func_index++].nome_funcao,temp);
            while(*expre!=')')expre++;
            expre++;
         } else buf_retorna();
      } else if(*token=='{')n_aspas++;
   } while(tipo_tok!=FINISHED);
   expre=t;
}
void dec_global(void) {
   pega_token();
   global_var_table[gvar_index].tipo=tipo_tok;
   global_var_table[gvar_index].valor=0;
   do {
      pega_token();
      strcpy(global_var_table[gvar_index++].p_variavel,token);
      pega_token();
   } while(*token==',');
   if(*token!=';') {
      erro_sintaxe(SEMI_ESPERADO);
   }
}
void dec_local(void) {
   struct table t;
   pega_token();
   t.tipo=tipo_tok;
   t.valor=0;
   do {
      pega_token();
      strcpy(t.p_variavel,token);
      local_push(t);
      pega_token();
   } while(*token==',');
   if(*token!=';') erro_sintaxe(SEMI_ESPERADO);
}
void pega_args(void) {
   int contador=0;
   double value,temp[NUM_PARAMS];
   struct table t;
   pega_token();
   if(*token!='(')erro_sintaxe(PARENT_ESPERADO);
   do {
      eval_expre(&value);
      temp[contador++]=value;
      pega_token();
   } while(*token==',');
   contador--;
   for(; contador>=0; contador--) {
      t.valor=temp[contador];
      t.tipo=ARGU;
      local_push(t);
   }
}
void pega_params(void) {
   struct table *t;
   int r=lvartos-1;
   do {
      pega_token();
      t=&local_var_stack[r];
      if(*token!=')') {
         if(tipo_tok!=INT&&tipo_tok!=CHAR)erro_sintaxe(TIPO_ESPERADO);
         t->tipo=tipo_token;
         pega_token();
         strcpy(t->p_variavel,token);
         pega_token(),r--;
      } else {
         break;
      }
   } while(*token==',');
   if(*token!=')')erro_sintaxe(PARENT_ESPERADO);
}
void func_ret(void) {
   double value=0;
   eval_expre(&value);
   retorno_valor=value;
}
int carrega_programa(char* filename) {
   FILE* LITTLE_C;
   if((LITTLE_C=fopen(filename,"r"))!=NULL) {
      int n=0;
      while(getc(LITTLE_C)!=EOF) n++;
      inicio_buf=malloc(n*sizeof(char)),rewind(LITTLE_C);
      for(int l=0; l<n; l++) inicio_buf[l]=getc(LITTLE_C);
      if(inicio_buf[n-1])inicio_buf[n-1]='\0';
      fclose(LITTLE_C);
      return 1;
   } else {
      printf("Arquivo não encontrado!\n");
      return 0;
   }
}
void call(void) {
   char* loc=find_func(token),*temp;
   int lvartemp;
   if(loc==NULL) {
      erro_sintaxe(FUNC_UNDEF);
   } else {
      lvartemp=lvartos;
      pega_args();
      temp=expre;
      func_push(lvartemp);
      expre=loc;
      pega_params();
      interpreta_bloco();/*Interpreta bloco*/
      expre=temp;
      lvartos=func_pop();
   }
}
void find_fim_do_bloco(void) {
   int b;
   pega_token();
   b=1;
   do {
      pega_token();
      if(*token=='{') {
         b++;
      } else if(*token=='}') {
         b--;
      }
   } while(b);
}
void exec_if(void) {
   double condicao;
   eval_expre(&condicao);
   if((int)condicao) {
      pega_token();
      interpreta_bloco();/*Interpreta bloco*/
   } else {
      pega_token();
      pega_token();
      find_fim_do_bloco();
      pega_token();
      if(tipo_tok!=ELSE) {
         buf_retorna();
         return;
      }
      interpreta_bloco();/*Interpreta bloco*/
   }
}
void exec_while(void) {
   double cond;
   char* temp;
   buf_retorna();
   temp=expre;
   pega_token();
   eval_expre(&cond);
   if((int)cond) {
      pega_token();
      interpreta_bloco();/*Interpreta bloco*/
   } else {
      pega_token();
      find_fim_do_bloco();
      return;
   }
   expre=temp;
}
void exec_do(void) {
   double cond;
   char* temp;
   buf_retorna();
   temp=expre;
   pega_token();
   interpreta_bloco();
   pega_token();
   if(tipo_tok!=WHILE)erro_sintaxe(WHILE_EXPECTED);
   eval_expre(&cond);/*verifica a condição do laço*/
   if((int)cond)expre=temp; /*se verdadeiro, repete. Se falso, continua*/
}
void exec_for(void) {
   double cond;
   char *temp,*temp2;
   int brace;
   pega_token();
   eval_expre(&cond);
   if(*token!=';') erro_sintaxe(SEMI_ESPERADO);
   expre++;
   temp=expre;
   for(;;) {
      eval_expre(&cond); /*Verifica a condição*/
      if(*token!=';')erro_sintaxe(SEMI_ESPERADO);
      expre++;
      temp2=expre;
      /*Acha o início do bloco do for*/
      brace=1;
      while(brace) {
         pega_token();
         if(*token=='(')brace++;
         if(*token==')')brace--;
      }
      pega_token();
      if((int)cond) {
         buf_retorna();
         interpreta_bloco();/*Interpreta bloco*/
      } else {/*Ignora o bloco*/
         find_fim_do_bloco();
         return;
      }
      expre=temp2;
      eval_expre(&cond);/*Efetua o incremento*/
      expre=temp;/*Volta para o início*/
   }
}
void interpreta_bloco(void) {
   int bloco=0;
   double value;
   do {
      tipo_token=pega_token();
      if(tipo_token==IDENTIFICADOR) {
         buf_retorna();
//         printf("eeeee\n");
         eval_expre(&value);
         if(*token!=';') printf("teste:%s\n",token), erro_sintaxe(SEMI_ESPERADO);
      } else if(tipo_token==BLOCO) {
         if(*token=='{')
            bloco=1;
         else return;
      } else {
         switch(tipo_tok) {
         case CHAR:
         case INT:
            buf_retorna();
            dec_local();
            break;
         case IF:
            exec_if();
            break;
         case RETURN :
            func_ret();
            return;
         case ELSE:
            find_fim_do_bloco();
            break;
         case FOR:
            exec_for();
            break;
         case DO:
            exec_do();
            break;
         case WHILE:
            exec_while();
            break;
//         case SWITCH:
//            break;
         case END:
            exit(0);
            break;
         }
      }
   } while(tipo_tok!=FINISHED&&bloco);
}
void atodou(double *value) {
   int i;
   switch(tipo_token) {
   case IDENTIFICADOR:
      i=procurar_inter_func(token);
      if(i!=-1)
         *value=funcoes_internas[i].p();
       else if(find_func(token)) {
         call();
         *value=retorno_valor;
      } else
         *value=encontra_var(token);
      pega_token();
      return;
   case NUMERO:///Verifica se é uma constante numérica
      *value=(double)atof(token);
      pega_token();
      return;
   case DELIMITADOR:
      if(*token=='\'') {
         *value=*expre++;
         if(*expre!='\'')erro_sintaxe(ASPAS_ESPERADO);
         expre++;
         pega_token();
      }
      return;
   default:
      if(*token==')')return;
      else erro_sintaxe(SINTAXE);
   }
}
void eval_exp5(double *value) {
   if((*token=='(')) {
      pega_token();
      eval_exp0(value);
      if(*token!=')')
         pega_token();
   } else atodou(value);
}
void eval_exp4(double *value) {
   char op='\0';
   if(*token=='+'||*token=='-') {
      op=*token,pega_token();
   }
   eval_exp5(value);
   if(op)
      if(op=='-')*value=-(*value);
}

void eval_exp3(double* value) {
   char op;
   double partial_value;
   int t;
   eval_exp4(value);
   while((op=*token)=='*'||op=='/'||op=='%') {
      pega_token();
      eval_exp4(&partial_value);
      switch(op) {
      case '*':
         *value=*value*partial_value;
         break;
      case '/':
         *value=*value/partial_value;
         break;
      case '%':
         t=(*value)/partial_value;
         *value=*value-(t*partial_value);
         break;
      }
   }
}
void eval_exp2(double* value) {
   char op;
   double valor_parcial=0;
   eval_exp3(value);
   while((op=*token)=='+'||op=='-') {
      pega_token();
      eval_exp3(&valor_parcial);
      switch(op) {
      case '-':
         *value=*value-valor_parcial;
         break;
      case '+':
         *value=*value+valor_parcial;
         break;
      }
   }
}
void eval_exp1(double* value) {
   double valor_parcial=0;
   char op;
   eval_exp2(value);
   op=*token;
   if(strchr(op_relacionais,op)) {
      pega_token();
      eval_exp2(&valor_parcial);
      switch(op) {
      case ME:
         *value=*value<valor_parcial;
         break;
      case MEI:
         *value=*value<=valor_parcial;
         break;
      case MA:
         *value=*value>valor_parcial;
         break;
      case MAI:
         *value=*value>=valor_parcial;
         break;
      case IG:
         *value=*value==valor_parcial;
         break;
      case NI:
         *value=*value!=valor_parcial;
         break;
      }
   }
}
void eval_exp0(double* value) {
   char temp[ID_TAM];
   register int temp_tok;
   if(tipo_token==IDENTIFICADOR) {
      if(se_variavel(token)) {
         strcpy(temp,token);
         temp_tok=tipo_token;
         pega_token();
         if(*token=='=') {
            pega_token();
            eval_exp0(value);
            atribui_variavel(temp,*value);
            return;
         } else {
            buf_retorna();
            strcpy(token,temp);
            tipo_token=temp_tok;
         }
      }
   }
   eval_exp1(value);
}
void eval_expre(double *value) {
   pega_token();
   if(!*token) {
      erro_sintaxe(NO_EXPRE);
      return;
   }
   if(*token==';') {
      *value=0;
      return;
   }
   eval_exp0(value);
   buf_retorna();
}
int pseudo_main(char* filename) {
   if(!carrega_programa(filename))exit(1);
   if(setjmp(ambiente_buffer))exit(1);
   expre=inicio_buf;
   pre_scan();
   gvar_index=lvartos=functos=0;
   expre=find_func("main");
   if(expre==NULL)exit(1);
   expre--;
   strcpy(token,"main");
   call();
   return 0;
}
int main(int argc, char *argv[]) {
   if(argc!=2) {
      printf("Número incorreto de parâmetros!\n");
      return 1;
   } else {
      printf("%s\n",argv[1]);
      pseudo_main(argv[1]);
   }
   free(inicio_buf);
   return 0;
}
