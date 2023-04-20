#include "BitSet.c"
#include "RubroMap.c"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
/// String builder usada apenas para armazenar o buffer da messagem
typedef struct {
  wchar_t *str;
  uint32_t cap;
  uint32_t sz;
} StringBuilder;
typedef struct HuffmanNode {
  wchar_t letra;
  uint32_t frequencia;
  struct HuffmanNode *parent;
  struct HuffmanNode *left;
  struct HuffmanNode *right;
  uint32_t Codigo;
} HuffmanNode;

typedef struct {
  HuffmanNode **Nodes;
  uint64_t size;
} PriorityQueueHeap;

HuffmanNode *newHuffmanNode(HuffmanNode *left, HuffmanNode *right) {
  HuffmanNode *novo = calloc(sizeof(HuffmanNode), 1);
  novo->left = left;
  novo->right = right;
  novo->frequencia = left->frequencia + right->frequencia;
  left->parent = right->parent = novo;
  return novo;
}
StringBuilder *message = NULL;
HuffmanNode **Folhas = NULL;
BitSet *bitset = NULL;
uint64_t nbits = 0;
HuffmanNode *Root = NULL;
uint32_t DictionarySize = 0;
// Liberar memória do StringBuilder
void StringBuilderClear(StringBuilder **strbuilder) {
  free((*strbuilder)->str);
  free(*strbuilder);
  *strbuilder = NULL;
}
void printMessage(void) {
  for (uint64_t i = 0; i < message->sz; i++)
    putwchar(message->str[i]);
  printf("Tamanho da mensagem: %u", message->sz);
}
// Cria um novo stringBuilder
StringBuilder *newStringBuilder() {
  StringBuilder *novo = calloc(sizeof(StringBuilder), 1);
  novo->cap = 128;
  novo->sz = 0;
  novo->str = calloc(sizeof(wchar_t), novo->cap);
  return novo;
}
// Concatena uma "string" em um stringBuilder
void StringBuilderAppend(StringBuilder *strbuilder, wchar_t wc) {
  if (strbuilder->sz + 1 >= strbuilder->cap) {
    strbuilder->cap *= 2;
    strbuilder->str =
        realloc(strbuilder->str, sizeof(wchar_t) * strbuilder->cap);
  }
  // Operação simples possível de paralelizar
  strbuilder->str[strbuilder->sz++] = wc;
}
void ClearPriorityQueue(PriorityQueueHeap **heap) {
  free((*heap)->Nodes);
  free(*heap);
}
void ClearHuffmanTree(HuffmanNode *node) {
  if (!node)
    return;
  ClearHuffmanTree(node->left);
  ClearHuffmanTree(node->right);
  free(node);
}
void GetFreqHelper(TreeNode *node, uint32_t *i, HuffmanNode **freq) {
  if (!node)
    return;
  GetFreqHelper(node->left, i, freq);
  freq[(*i)] = calloc(sizeof(HuffmanNode), 1);
  freq[(*i)]->frequencia = node->value;
  freq[(*i)]->letra = node->key;
  (*i)++;
  GetFreqHelper(node->right, i, freq);
}
HuffmanNode **GetFreq(TreeMap *map) {
  HuffmanNode **freq = calloc(sizeof(HuffmanNode *), map->size);
  uint32_t i = 0;
  GetFreqHelper(map->root, &i, freq);
  return freq;
}
void PriorityQueueInsert(PriorityQueueHeap *heap, HuffmanNode *k) {
  uint32_t s = heap->size++, f = s / 2;
  while (s > 0 && heap->Nodes[f]->frequencia > k->frequencia) {
    heap->Nodes[s] = heap->Nodes[f];
    s = f, f /= 2;
  }
  heap->Nodes[s] = k;
}

HuffmanNode *PriorityQueueGetMin(PriorityQueueHeap *heap) {
  if (heap->size == 0) {
    printf("Heap vazia\n");
    exit(4);
  }
  int s, f;
  int i = --heap->size;
  HuffmanNode *ret = heap->Nodes[0];
  if (i > 0) {
    HuffmanNode *v = heap->Nodes[i];
    heap->Nodes[i] = heap->Nodes[0];
    f = 0, s = i == 1 ? -1 : 1;
    if (i > 2 && heap->Nodes[2]->frequencia < heap->Nodes[1]->frequencia)
      s = 2;
    while (s >= 0 && v->frequencia > heap->Nodes[s]->frequencia) {
      heap->Nodes[f] = heap->Nodes[s];
      f = s, s = f * 2 + 1;
      if (s + 1 <= i - 1 &&
          heap->Nodes[s]->frequencia > heap->Nodes[s + 1]->frequencia)
        s += 1;
      if (s > i - 1)
        s = -1;
    }
    heap->Nodes[f] = v;
  }
  return ret;
}
void GenFrequencyOfSimbols(void) {
  TreeMap *map = NewTreeMap();
  for (uint32_t i = 0; i < message->sz; i++)
    InsertTreeMap(map, message->str[i], 1);
  InsertTreeMap(map, '\0', 1);
  Folhas = GetFreq(map);
  DictionarySize = map->size;
  ClearMap(&map);
}
void AtribuiCodigo(HuffmanNode *node) {
  // O bit mais significativo marca o cumprimento do do codigo
  uint32_t c = 1U;
  HuffmanNode *t = node;
  while (t->parent != NULL) {
    c = (c << 1U) | (t->parent->right == t);
    t = t->parent;
  }
  node->Codigo = c;
}
void GenHuffmanTree(void) {
  PriorityQueueHeap *heap = calloc(sizeof(PriorityQueueHeap), 1);
  heap->Nodes = calloc(sizeof(HuffmanNode *), DictionarySize);
  for (uint32_t i = 0; i < DictionarySize; i++)
    PriorityQueueInsert(heap, Folhas[i]);
  while (heap->size >= 2) {
    HuffmanNode *left = PriorityQueueGetMin(heap);
    HuffmanNode *right = PriorityQueueGetMin(heap);
    PriorityQueueInsert(heap, newHuffmanNode(left, right));
  }
  Root = PriorityQueueGetMin(heap);
  ClearPriorityQueue(&heap);
}
void GenBitSet() {
  TreeMap *map = NewTreeMap();
  for (uint32_t i = 0; i < DictionarySize; i++)
    InsertTreeMap(map, Folhas[i]->letra, Folhas[i]->Codigo);
  TreeNode *nd;
  uint32_t cd;
  uint64_t c = 0;
  bitset = newBitSet();
  for (uint32_t i = 0; i <= message->sz; i++) {
    if ((nd = GetFromMap(map, message->str[i])) == NULL) {
      printf("Erro ao mapear cada letra em codigo\n");
      exit(7);
    }
    cd = nd->value;
    while (cd > 1) {
      if (cd & 1UL)
        BitsetPut(bitset, c);
      cd >>= 1;
      c++;
    }
  }
  bitset->idx = c;
  ClearMap(&map);
}
void WriteEncodedMessage(char *outputFile) {
  FILE *fp = fopen(outputFile, "wb");
  if (!fp) {
    printf("Erro ao abrir o arquivo\n");
    exit(4);
  }
  wchar_t *alfa = malloc(sizeof(wchar_t) * DictionarySize);
  uint32_t *freq = malloc(sizeof(uint32_t) * DictionarySize);
  for (uint32_t i = 0; i < DictionarySize; i++) {
    alfa[i] = Folhas[i]->letra;
    freq[i] = Folhas[i]->frequencia;
  }
  fwrite(&DictionarySize, sizeof(DictionarySize), 1, fp);
  fwrite(alfa, sizeof(wchar_t), DictionarySize, fp);
  fwrite(freq, sizeof(uint32_t), DictionarySize, fp);
  fwrite(&bitset->idx, sizeof(uint64_t), 1, fp);
  fwrite(bitset->bits, sizeof(uint64_t), bitset->size, fp);
  fclose(fp);
  free(alfa);
  free(freq);
  free(Folhas);
  ClearBitSet(bitset);
  ClearHuffmanTree(Root);
}
void EncodeMessage(char *inputFile, char *outputFile) {
  FILE *fp = fopen(inputFile, "r");
  if (!fp) {
    printf("Erro ao abrir arquivo\n");
    exit(3);
  }
  message = newStringBuilder();
  wchar_t wc;
  while ((wc = getwc(fp)) != EOF)
    StringBuilderAppend(message, wc);
  fclose(fp);
  GenFrequencyOfSimbols();
  GenHuffmanTree();
  for (uint32_t i = 0; i < DictionarySize; i++)
    AtribuiCodigo(Folhas[i]);
  GenBitSet();
  StringBuilderClear(&message);
  WriteEncodedMessage(outputFile);
}

void GenMessage(char *outputFile) {
  FILE *fp;
  if ((fp = fopen(outputFile, "w")) == NULL) {
    printf("Erro ao abrir arquivo\n");
    exit(32);
  }
  HuffmanNode *t = Root;
  uint64_t i = 0;
  while (i < bitset->idx) {
    bool b = BitsetGet(bitset, i);
    if (t->left == NULL && t->right == NULL) {
      if (WEOF == fputwc(t->letra, fp))
        exit(12);
      t = Root;
    }
    t = b ? t->right : t->left;
    i++;
  }
  fclose(fp);
  ClearBitSet(bitset);
  ClearHuffmanTree(Root);
  free(Folhas);
}
void DecodeMessage(char *inputFile, char *outputFile) {
  FILE *fp = fopen(inputFile, "rb");
  if (!fp) {
    printf("Erro ao abrir arquivo\n");
    exit(3);
  }
  bitset = calloc(sizeof(BitSet), 1);
  fread(&DictionarySize, sizeof(DictionarySize), 1, fp);
  wchar_t *alfa = calloc(sizeof(wchar_t), DictionarySize);
  uint32_t *freq = calloc(sizeof(uint32_t), DictionarySize);
  fread(alfa, sizeof(wchar_t), DictionarySize, fp);
  fread(freq, sizeof(uint32_t), DictionarySize, fp);
  fread(&bitset->idx, sizeof(uint64_t), 1, fp);
  bitset->size = bitset->idx / M_ULONG + 1;
  bitset->bits = calloc(sizeof(uint64_t), bitset->size);
  fread(bitset->bits, sizeof(uint64_t), bitset->size, fp);
  fclose(fp);
  Folhas = calloc(sizeof(HuffmanNode *), DictionarySize);
  for (uint32_t i = 0; i < DictionarySize; i++) {
    Folhas[i] = calloc(sizeof(HuffmanNode), 1);
    Folhas[i]->letra = alfa[i];
    Folhas[i]->frequencia = freq[i];
  }
  free(alfa);
  free(freq);
  GenHuffmanTree();
  GenMessage(outputFile);
}
void printBitset(BitSet *set) {
  for (uint64_t i = 0; i < set->size; i++) {
    printf("%lu\n", set->bits[i]);
  }
  printf("\n");
}
int main(int argc, char *argv[]) {
  setlocale(LC_CTYPE, "");
  const char *erro_msg = {"Erro de sintaxe!\n\n\
             \rExemplos de sintaxe:\n\
             \r\n%s -e arquivo-para-codificar.txt destino.txt\n\
             \r\n%s -d arquivo-codificado.txt     destino.txt\n"};
  if (argc != 4) {
    printf(erro_msg, strrchr(argv[0], '/') + 1, strrchr(argv[0], '/') + 1);
    return 1;
  }
  int s = -1;
  char param[2][3] = {"-e", "-d"};
  for (int a = 0; a < 2; a++)
    if (!strcmp(param[a], argv[1]))
      s = a;
  if (s == -1)
    printf(erro_msg, strrchr(argv[0], '/') + 1, strrchr(argv[0], '/') + 1);
  if (!strcmp(argv[2], argv[3])) {
    printf("os arquivos tem de ser diferentes!\n"), s = -1;
    return 2;
  }
  switch (s) {
  case 0:
    EncodeMessage(argv[2], argv[3]);
    break;
  case 1:
    DecodeMessage(argv[2], argv[3]);
    break;
  }
  return 0;
}
