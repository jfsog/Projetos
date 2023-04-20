/* Implementação da arvore rubro negra */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#define BLACK true
#define RED false
typedef struct Node {
  struct Node *left;
  struct Node *right;
  wchar_t key;
  uint32_t value;
  bool cor;
} TreeNode;
typedef struct {
  TreeNode *root;
  uint32_t size;
} TreeMap;

int isBlack(TreeNode *);
void ClearMap(TreeMap **map);
TreeMap *NewTreeMap(void) { return calloc(sizeof(TreeMap), 1); }
TreeNode *clearHelper(TreeNode *node) {
  if (node != NULL) {
    node->left = clearHelper(node->left);
    node->right = clearHelper(node->right);
    free(node);
  }
  return NULL;
}
void ClearMap(TreeMap **map) {
  if (*map == NULL)
    return;
  (*map)->root = clearHelper((*map)->root);
  free(*map);
  *map = NULL;
}
TreeNode *newNode(wchar_t key, uint32_t frequencia) {
  TreeNode *new = calloc(sizeof(TreeNode), 1);
  new->key = key;
  new->value = frequencia;
  new->cor = RED;
  return new;
}
int isBlack(TreeNode *node) {
  if (node == NULL)
    return BLACK;
  return node->cor;
}
TreeNode *rotateLeft(TreeNode *node) {
  TreeNode *t1 = node->right;
  node->right = t1->left;
  t1->left = node;
  return t1;
}
TreeNode *rotateRight(TreeNode *node) {
  TreeNode *t1 = node->left;
  node->left = t1->right;
  t1->right = node;
  return t1;
}
void TioVermelho(TreeNode *avo) {
  avo->cor = RED;
  avo->left->cor = avo->right->cor = BLACK;
}
TreeNode *Caso_EsqEsq(TreeNode *node) {
  node->cor = RED;
  node->left->cor = BLACK;
  return rotateRight(node);
}
TreeNode *Caso_DirDir(TreeNode *node) {
  node->cor = RED;
  node->right->cor = BLACK;
  return rotateLeft(node);
}
TreeNode *AjustInsert(TreeNode *node) {
  if (!isBlack(node->left)) {
    TreeNode *tio = node->right;
    if (!isBlack(node->left->left)) {
      if (!isBlack(tio)) {
        TioVermelho(node);
        return node;
      } else {
        return Caso_EsqEsq(node);
      }
    } else if (!isBlack(node->left->right)) {
      if (!isBlack(tio)) {
        TioVermelho(node);
        return node;
      } else {
        node->left = rotateLeft(node->left);
        return Caso_EsqEsq(node);
      }
    }
  }
  if (!isBlack(node->right)) {
    TreeNode *tio = node->left;
    if (!isBlack(node->right->right)) {
      if (!isBlack(tio)) {
        TioVermelho(node);
      } else {
        return Caso_DirDir(node);
      }
    } else if (!isBlack(node->right->left)) {
      if (!isBlack(tio)) {
        TioVermelho(node);
      } else {
        node->right = rotateRight(node->right);
        return Caso_DirDir(node);
      }
    }
  }
  return node;
}
TreeNode *insertHelper(TreeNode *node, TreeMap *map, wchar_t key,
                       uint32_t frequencia) {
  if (node == NULL) {
    map->size++;
    return newNode(key, frequencia);
  }
  if (key > node->key)
    node->right = insertHelper(node->right, map, key, frequencia);
  else if (key < node->key)
    node->left = insertHelper(node->left, map, key, frequencia);
  else {
    // Usado no algotitimo de huffman para armazenar a frequencia ou o código da
    // palavra
    node->value++;
    return node;
  }
  return AjustInsert(node);
}
void InsertTreeMap(TreeMap *map, wchar_t key, uint32_t frequencia) {
  map->root = insertHelper(map->root, map, key, frequencia);
  map->root->cor = BLACK;
}
bool TreeMapContains(TreeNode *node, wchar_t key) {
  if (node == NULL)
    return false;
  if (key < node->key)
    return TreeMapContains(node->left, key);
  if (key > node->key)
    return TreeMapContains(node->right, key);
  return true;
}
TreeNode *pegaSucessor(TreeNode *node) {
  TreeNode *t = node;
  while (t->left != NULL)
    t = t->left;
  return t;
}
TreeNode *AjustRemocao(TreeNode *pai, TreeNode *node, bool *dupPreto) {
  if (!(*dupPreto))
    return pai;
  bool isEsq = pai->right == node;
  TreeNode *irmao = isEsq ? pai->left : pai->right;
  if (isBlack(pai)) {
    if (isBlack(irmao)) {
      if (isBlack(irmao->left)) {
        if (isBlack(irmao->right)) {
          irmao->cor = RED;
        } else {
          if (isEsq) {
            pai->left = rotateLeft(pai->left);
            pai = rotateRight(pai);
            pai->cor = BLACK;
          } else {
            irmao->right->cor = BLACK;
            pai = rotateLeft(pai);
          }
          *dupPreto = false;
        }
      } else {
        if (isBlack(irmao->right)) {
          irmao->left->cor = BLACK;
          if (isEsq) {
            pai = rotateRight(pai);
          } else {
            pai->right = rotateRight(pai->right);
            pai = rotateLeft(pai);
          }
          *dupPreto = false;
        } else {
          if (isEsq) {
            pai->left = rotateLeft(pai->left);
            pai = rotateRight(pai);
            pai->left->cor = BLACK;
            pai->cor = BLACK;
          } else {
            pai = rotateLeft(pai);
            pai->right->cor = BLACK;
          }
          *dupPreto = false;
        }
      }
    } else if (isBlack(irmao->left) && isBlack(irmao->right)) {
      if (isEsq) {
        pai = Caso_EsqEsq(pai);
        pai->right = AjustRemocao(pai->right, node, dupPreto);
      } else {
        pai = Caso_DirDir(pai);
        pai->left = AjustRemocao(pai->left, node, dupPreto);
      }
      *dupPreto = false;
    }
  } else if (isBlack(irmao)) {
    if (isBlack(irmao->left)) {
      if (isBlack(irmao->right)) {
        irmao->cor = RED;
        pai->cor = BLACK;
        *dupPreto = false;
      } else {
        if (isEsq) {
          pai->left = Caso_DirDir(irmao);
          pai = AjustRemocao(pai, node, dupPreto);
        } else {
          pai = rotateLeft(pai);
          *dupPreto = false;
        }
      }
    } else {
      if (isBlack(irmao->right)) {
        pai->cor = BLACK;
        if (isEsq) {
          pai = rotateRight(pai);
          pai->left->cor = BLACK;
        } else {
          pai->right = rotateRight(pai->right);
          pai = rotateLeft(pai);
        }
        pai->cor = RED;
        *dupPreto = false;
      } else {
        pai->cor = BLACK;
        irmao->cor = RED;
        if (isEsq) {
          pai = rotateRight(pai);
          pai->left->cor = BLACK;
        } else {
          pai = rotateLeft(pai);
          pai->right->cor = BLACK;
        }
        *dupPreto = false;
      }
    }
  }
  return pai;
}
TreeNode *RemoveHelper(TreeNode *node, TreeNode **son, wchar_t key,
                       bool *DoubleBlack) {
  if (key > node->key)
    node->right = RemoveHelper(node->right, son, key, DoubleBlack);
  else if (key < node->key) {
    node->left = RemoveHelper(node->left, son, key, DoubleBlack);
  } else {
    if (node->left != NULL && node->right != NULL) {
      TreeNode *NoSucessor = pegaSucessor(node->right);
      node->right =
          RemoveHelper(node->right, son, NoSucessor->key, DoubleBlack);
      node->key = NoSucessor->key;
      node->value = NoSucessor->value;
      *son = node->right;
    } else {
      TreeNode *temp = node;
      if (node->left == NULL && node->right == NULL) {
        (*DoubleBlack) = isBlack(node);
        node = NULL;
      } else {
        if (node->left == NULL)
          node = node->right;
        else
          node = node->left;
        node->cor = BLACK;
      }
      free(temp);
      return *son = node;
    }
  }
  return (*son = AjustRemocao(node, *son, DoubleBlack));
}
TreeNode *GetFromMap(TreeMap *map, wchar_t key) {
  TreeNode *N = map->root;
  while (N) {
    if (key == N->key)
      return N;
    if (key > N->key)
      N = N->right;
    else
      N = N->left;
  }
  return NULL;
}

void RemoveTreeMap(TreeMap *tree, wchar_t key) {
  bool cond = false;
  TreeNode *temp = NULL;
  tree->root = RemoveHelper(tree->root, &temp, key, &cond);
  if (tree->root)
    tree->root->cor = BLACK;
}
