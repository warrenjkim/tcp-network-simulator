#include "utils.h"

// node
Node *node_init(const struct packet *pkt) {
    Node *node = (Node *)(malloc(sizeof(Node)));
    if (!node) {
        perror("node initialization failed");
        exit(1);
    }

    node->id = pkt->acknum;
    node->pkt =  *pkt;
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->color = RED;

    return node;
}


void node_destroy(Node *node) {
    if (node)
        free(node);
}



// red black tree

Tree *rbt_init() {
    Tree *tree = (Tree *)(malloc(sizeof(Tree)));
    if (!tree) {
        perror("tree initialization failed");
        exit(1);
    }
    tree->size = 0;
    tree->root = NULL;

    return tree;
}


Node *rbt_insert(Node *root, struct packet *pkt) {
    if (!root)
        return node_init(pkt);

    if (root->id < pkt->acknum)
        root->left = rbt_insert(root->left, pkt);
    else if (pkt->acknum < root->id)
        root->right = rbt_insert(root->right, pkt);
    
    root = rbt_balance(root);
    root->color = BLACK;

    return root;
}


Node *rbt_delete(Node *root, const unsigned short id) {
    if (!root)
        return NULL;

    if (root->id < id)
        root->left = rbt_delete(root->left, id);
    else if (id < root->id)
        root->right = rbt_delete(root->right, id);

    if (!root->left || !root->right) {
        Node *temp = root->left ? root->left : root->right;

        if (!temp) {
            temp = root;
            root = NULL;
        }
        else {
            Node *del = root;
            *root = *temp;
            node_destroy(del);
        }
    } 
    else {
        Node *successor = rbt_successor(root->right);
        root->pkt = successor->pkt;
        root->right = rbt_delete(root->right, successor->id);
    }

    if (!root)
        return NULL;
    
    return rbt_balance(root);
}


Node *rbt_successor(Node *root) {
    if (!root->left)
        return root;
    return rbt_successor(root->left);
}


Node *grandparent(Node *node) {
    return node->parent->parent;
}


Node *uncle(Node *node) {
    return grandparent(node)->right;
}


Node *rbt_balance(Node *root) {
    // case 1 (double red)
    if (grandparent(root)->color == RED && root->parent->color == RED)
        recolor();
    else {

    }
    return NULL;
}


void rbt_ll_rotate(Node *X) {
    Node *Y = X->right;
    Node *T = Y->left;

    X->right = T;
    Y->left = X;

    Y->parent = X->parent;
    X->parent = Y;
    
    if (T)
        T->parent = X;

    if (Y->parent) {
        if (Y->parent->left == X)
            Y->parent->left = Y;
        else
            Y->parent->right = Y;
    }
}


Node *rbt_lr_rotate(Node *root) {
    return NULL;
}


void rbt_inorder(Node *root) {
    if (!root) {
        return;
    }

    rbt_inorder(root->left); 
    printf("acknum: %d\n", root->pkt.acknum);
    rbt_inorder(root->right); 
}
