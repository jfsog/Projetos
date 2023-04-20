#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#define MAXBITS 50
#define DATA_SIZE (sizeof(unsigned int)*8-1)

void hfencode_write(char*);
void print_msg(void);

size_t n_simbolos=0;
void **to_free=NULL;
size_t fsize=0;
void push_to_free(void* tofree) {
   ++fsize;
   to_free=realloc(to_free,sizeof(void*)*fsize);
   to_free[fsize-1]=tofree;
}
typedef struct {
   _Bool bits[MAXBITS];
   short int startpos;
} codetype;

typedef struct tipo_no {
   size_t freq;
   wchar_t c;
   size_t nopai;
   _Bool isleft;
   struct tipo_no* esqPtr;
   struct tipo_no* dirPtr;
} nodetype;

nodetype* nodes=NULL;
codetype* codes=NULL;

struct node_priority {
   struct node_priority* proxPrt;
   int index;
   nodetype* data;
};
struct node_priority *head=NULL;
size_t root=0;

typedef struct catalogo {
   wchar_t simbolo;
   unsigned int frequencia;
} catalogo;
catalogo* alfabeto=NULL;

typedef struct ALPHA_LIST {
   struct ALPHA_LIST* prox;
   catalogo data;
} ALPHA_LIST;
ALPHA_LIST* list_alpha=NULL;
///Estrutura usada para criar uma fila que representa a mensagem
typedef struct MSG_L {
   wchar_t letter;
   struct MSG_L* prox;
} MSG_L;
MSG_L* ini_mensagem=NULL,*fim_mensagem=NULL;

typedef struct PQHEAP {
   int* minheap;
   int size;
} PQHEAP;
PQHEAP heap_pq;
void pqinserir(int k) {
   int s=heap_pq.size++,f=(s)/2;
   while(s>0 && nodes[heap_pq.minheap[f]].freq > nodes[k].freq) {
      heap_pq.minheap[s]=heap_pq.minheap[f];
      s=f,f=(f)/2;
   }
   heap_pq.minheap[s]=k;
}
int pqdelete(void) {
   if(heap_pq.size==0) return -1;
   int s,i,f,value,ret;
   i=--heap_pq.size;
   ret=heap_pq.minheap[0];
   if(i>0) {
      value=heap_pq.minheap[i],
      heap_pq.minheap[i]=heap_pq.minheap[0];
      f=0,s=i==1?-1:1;
      if(i>2 && nodes[heap_pq.minheap[2]].freq < nodes[heap_pq.minheap[1]].freq) s=2;
      while(s>=0 && nodes[value].freq > nodes[heap_pq.minheap[s]].freq) {
         heap_pq.minheap[f]=heap_pq.minheap[s];
         f=s,s=2*f+1;
         if(s+1<=i-1 && nodes[heap_pq.minheap[s]].freq > nodes[heap_pq.minheap[s+1]].freq)
            s+=1;
         if(s>i-1)
            s=-1;
      }
      heap_pq.minheap[f]=value;
   }
   return ret;
}

///Push usado para criar uma lista de prioridades
void pushq(size_t i) {
   struct node_priority*atual=head,*anterior=NULL,*novo=NULL;
   while(atual&&nodes[i].freq>atual->data->freq) {
      anterior=atual;
      atual=atual->proxPrt;
   }
   novo=malloc(sizeof(struct node_priority));
   push_to_free(novo);
   novo->index=i;
   novo->data=&nodes[i];
   if(anterior==NULL) {
      novo->proxPrt=head;
      head=novo;
   } else {
      novo->proxPrt=anterior->proxPrt;
      anterior->proxPrt=novo;
   }
}

///Cria uma representação do percurso da descida durante a subina na arvore
void up_tree(void) {
   for(int a=0,b=0; a<n_simbolos; a++) {
      int index=b=a;
      codes[b].startpos=MAXBITS;
      while(index!=root) {
         --codes[b].startpos;
         codes[b].bits[codes[b].startpos]=nodes[index].isleft?0:1;
         index=nodes[index].nopai;
      }
   }
}
///Atribui se um nó é filho à esquerda ou à direita de outro nó
void map_tree(void) {
   size_t atual=0,anterior=-1;
   for(int i=0; i<n_simbolos; i++) {
      atual=i;
      while(atual!=root) {
         anterior=atual;
         atual=nodes[atual].nopai;
         if(nodes[anterior].isleft)
            nodes[atual].esqPtr=&nodes[anterior];
         else
            nodes[atual].dirPtr=&nodes[anterior];
      }
   }
   up_tree();
}
/*
   Função que da nome ao programa. Aqui é gerado a arvore de Huffman
   com base na fila de prioridades dos nós iniciais
   (nós folhas compostos pelo alfabeto e a frequencia de cada simbolo)
*/
void gen_tree(void) {
   int p1,p2,p,b;
   for(p=n_simbolos,b=2*n_simbolos-1; p<b; p++) {
      p1=pqdelete();
      p2=pqdelete();
      if(p1<0||p2<0)exit(8);
      nodes[p1].nopai=p;
      nodes[p1].isleft=1;
      nodes[p2].nopai=p;
      nodes[p2].isleft=0;
      nodes[p].freq=nodes[p1].freq+nodes[p2].freq;
      nodes[p].esqPtr=nodes[p].dirPtr=NULL;
      nodes[p].c=-1;
      pqinserir(p);

   }
   root=pqdelete();
   if(pqdelete()!=-1)exit(8);
   map_tree();
}
/*
   Inicializa os vetores nodes e codes,
   nodes servirá para representar a árvore,
   enquanto codes conterá os símbolos e sua representação na codificação de Huffman
*/
void print_heap(int i,int profun) {
   if(i<heap_pq.size) {
      print_heap(i*2+2,profun+1);
      for(int a=0; a<profun*5; a++)
         printf(" ");
      printf("%3d\n",nodes[heap_pq.minheap[i]].freq);
      print_heap(i*2+1,profun+1);
   }
}
void gen_freq(void) {
   size_t a=0,n;
   nodes=calloc(sizeof(nodetype),(2*n_simbolos-1));
   push_to_free(nodes);
   codes=calloc(sizeof(codetype),n_simbolos);
   heap_pq.minheap=calloc(sizeof(int),(2*n_simbolos-1));
   push_to_free(heap_pq.minheap);
   push_to_free(codes);
   for(a=0; a<2*n_simbolos-1; a++)
      nodes[a].c=-1;
   ALPHA_LIST* temp=list_alpha;
   n=0;
   while(temp) {
      nodes[n].freq=temp->data.frequencia;
      nodes[n].c=temp->data.simbolo;
      nodes[n].esqPtr=nodes[n].dirPtr=NULL;
      pqinserir(n);
      n++,temp=temp->prox;
   }
//   print_heap(0,0);
   gen_tree();
}
unsigned int *v_codes=NULL;
unsigned int v_code_size=0;
unsigned int v_bit_lengh=DATA_SIZE;
size_t msg_size=0;
void gen_code_huff(int i) {
   /*
      v_codes é alocado com o tamanho da mensagem lida
   */
   if(!v_codes)v_codes=calloc(sizeof(unsigned int),msg_size),push_to_free(v_codes);
   for(int a=codes[i].startpos; a<MAXBITS; a++) {
      v_codes[v_code_size]=v_codes[v_code_size]+((codes[i].bits[a])<<v_bit_lengh);
      if(v_bit_lengh) {
         v_bit_lengh--;
      } else {
         v_bit_lengh=DATA_SIZE;
         v_code_size++;
      }
   }
}

void encode(void) {
   gen_freq();
   for(MSG_L* t=ini_mensagem; t; t=t->prox)
      for(int a=0; a<n_simbolos; a++)
         if(t->letter==nodes[a].c)
            gen_code_huff(a);
   v_code_size++;
}
void push_alfa(wchar_t letra) {
   ALPHA_LIST* atual=list_alpha,*anterior=NULL;
   while(atual&&letra>atual->data.simbolo)
      anterior=atual, atual=atual->prox;
   if(atual&&atual->data.simbolo==letra) {
      atual->data.frequencia++;
   } else {
      n_simbolos++;
      ALPHA_LIST* novo=calloc(sizeof(catalogo),1);
      push_to_free(novo);
      novo->data.simbolo=letra;
      novo->data.frequencia++;
      if(!anterior)
         novo->prox=list_alpha,    list_alpha=novo;
      else
         novo->prox=anterior->prox, anterior->prox=novo;
   }
}
void push_msg(wchar_t letra) {
   MSG_L* novo=malloc(sizeof(MSG_L)),*t=NULL;
   push_to_free(novo);
   novo->letter=letra;
   novo->prox=NULL;
   msg_size++;
   if(!ini_mensagem)
      ini_mensagem=fim_mensagem=novo;
   else
      fim_mensagem->prox=novo,   fim_mensagem=novo;

}
void read_to_encode(char *origem,char* destino) {
   FILE* file;
   if((file=fopen(origem,"r"))) {
      wchar_t c;
      while((c=getwc(file))!=WEOF) push_msg(c);
      push_msg('\0');
      MSG_L*t=ini_mensagem;
      while(t)
         push_alfa(t->letter), t=t->prox;
      fclose(file);
   } else printf("nulo"),exit(6);
   encode();
   hfencode_write(destino);
}
void hfencode_write(char* destino) {
   FILE *fp;
   fp = fopen(destino, "wb" );
   fwrite(&n_simbolos, sizeof(unsigned int), 1, fp );
   ALPHA_LIST* temp=list_alpha;
   alfabeto=calloc(sizeof(catalogo),n_simbolos);
   push_to_free(alfabeto);
   for(int a=0; a<n_simbolos; a++) {
      alfabeto[a].frequencia=temp->data.frequencia;
      alfabeto[a].simbolo=temp->data.simbolo;
      temp=temp->prox;
   }
   fwrite(alfabeto, sizeof(catalogo), n_simbolos, fp );
   fwrite(&v_code_size, sizeof(unsigned int), 1, fp );
   for(unsigned int a=0; a<v_code_size; a++)
      fwrite(&v_codes[a], sizeof(unsigned int), 1, fp );
   fclose(fp);
}
void print_msg(void) {
   MSG_L* temp=ini_mensagem;
   while(temp)
      printf("%lc",temp->letter),temp=temp->prox;
}
/*
   Decodifica a menssagem que esta representada no array de v_codes
*/
void huffman_decode(void) {
   nodetype* raiz=&nodes[root],*atual=NULL,*anterior;
   int bit_sz=DATA_SIZE;
   _Bool status=0;
   if(ini_mensagem==NULL)status=1;
   for(size_t index=0; index<v_code_size;) {
      atual=raiz;
      anterior=NULL;
      while(atual) {
         anterior=atual;
         if(v_codes[index]&(1<<(bit_sz))?1:0)
            atual=atual->dirPtr;
         else
            atual=atual->esqPtr;
         if(atual) bit_sz--;
         if(bit_sz==-1)bit_sz=DATA_SIZE,index++;
      }
      if(status)
         push_msg(anterior->c);
      /*
         O laço acaba quando o caractere nulo é encontrado
      */
      if(anterior->c=='\0'||(atual&&atual->c=='\0'))  break;
   }
}
void read_to_decode(char* origem,char* destino) {
   FILE* fp;
   if((fp=fopen(origem,"rb"))!=NULL) {
      fread(&n_simbolos,sizeof(unsigned int),1,fp);
      alfabeto=calloc(sizeof(catalogo),n_simbolos);
      push_to_free(alfabeto);
      fread(alfabeto,sizeof(catalogo),n_simbolos,fp);
      fread(&v_code_size, sizeof(unsigned int), 1, fp );
      v_codes=calloc(sizeof(unsigned int),v_code_size);
      push_to_free(v_codes);
      fread(v_codes, sizeof(unsigned int), v_code_size, fp );
      fclose(fp);
   } else printf("Arquivo não existe!\n"),exit(6);
   size_t temp_n_simb=n_simbolos;
   for(size_t a=0; a<temp_n_simb; a++)
      for(size_t b=0; b<alfabeto[a].frequencia; b++)
         push_alfa(alfabeto[a].simbolo);
   n_simbolos=temp_n_simb;
   gen_freq();
   huffman_decode();
   if((fp=fopen(destino,"w"))!=NULL) {
      for (MSG_L* i =ini_mensagem; i; i=i->prox)
         if (WEOF == fputwc(i->letter, fp))
            exit(1);
      fclose(fp);
   } else printf("Erro ao escrever no arquivo!\n");
}
void free_mem(void) {
   for(int i=0; i<fsize; i++) free(to_free[i]);
   if(fsize) free(to_free);
}
int main(int argc,char* argv[]) {
   setlocale(LC_CTYPE,"");
   char* erro_msg= {"Erro de sintaxe!\n\n\
             \rExemplos de sintaxe:\n\
             \r\n%s -e arquivo-para-codificar.txt destino.txt\n\
             \r\n%s -d arquivo-codificado.txt     destino.txt\n"
                   };
   if(argc!=4) {
      printf(erro_msg,strrchr(argv[0],'/')+1,strrchr(argv[0],'/')+1);
      return 1;
   }
   int s=-1;
   char param[2][3]= {"-e","-d"};
   for(int a=0; a<2; a++)
      if(!strcmp(param[a],argv[1]))s=a;
   if(s==-1)
      printf(erro_msg);
   if(!strcmp(argv[2],argv[3])) {
      printf("os arquivos tem de ser diferentes!\n"),s=-1;
      return 2;
   }
   switch(s) {
   case 0:
      read_to_encode(argv[2],argv[3]);
      break;
   case 1:
      read_to_decode(argv[2],argv[3]);
      break;
   }
   print_msg();
   free_mem();
   return 0;
}
