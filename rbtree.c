#include "utils.h"

// node
Node *node_init(const struct packet *pkt) {
    Node *node = (Node *)(malloc(sizeof(Node)));
    if (!node) {
        perror("node initialization failed");
        exit(1);
    }

    node->id = pkt->acknum;
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


Node *node_insert(Node *root, struct packet *pkt, Node **Z) {
    if (!root) {
        *Z = node_init(pkt);
        return *Z;
    }
    if (pkt->acknum < root->id) {
        root->left = node_insert(root->left, pkt, Z);
        root->left->parent = root;
    }
    else if (root->id < pkt->acknum) {
        root->right = node_insert(root->right, pkt, Z);
        root->right->parent = root;
    }


    return root;
}


Node *rbt_replacement(Node *root) {
    if (root->left && root->right)
        return rbt_successor(root->right);

    if (!root->left && !root->right)
        return NULL;

    return root->left ? root->left : root->right;
}


Node *node_delete(Node *root, Node *V) {
    Node *U = rbt_replacement(V);
    Node *parent = V->parent;
    bool double_black = (!U || U->color == BLACK) && (V->color == BLACK);

    if (!U) {
        if (V == root)
            root = NULL;
        else {
            if (double_black)
                root = rbt_double_black(root, V);
            else {
                Node *sibling = rbt_sibling(V);
                if (sibling)
                    sibling->color = RED;
            }

            if (parent) {
                if (parent->left == V)
                    parent->left = NULL;
                else
                    parent->right = NULL;
            }

            node_destroy(V);
            return root;
        }
    }

    if (!V->left || !V->right) {
        if (V == root) {
            V->id = U->id;
            V->left = V->right = NULL;
            free(U);
        }
        else if (parent) {
            if (parent->left == V)
                parent->left = U;
            else
                parent->right = U;

            node_destroy(V);
            U->parent = parent;

            if (double_black)
                root = rbt_double_black(root, U);
            else
                U->color = BLACK;
        }
        return root;
    }

    unsigned short temp = U->id;
    U->id = V->id;
    V->id = temp;

    return node_delete(root, U);
}


Node *rbt_double_black(Node *root, Node *X) {
    if (X == root) {
        root->color = BLACK;
        return root;
    }

    Node *parent = X->parent;
    Node *sibling = (X == parent->left) ? parent->right : parent->left;

    if (sibling && sibling->color == RED) {
        parent->color = RED;
        sibling->color = BLACK;
        if (X == parent->left) {
            root = rbt_ll_rotate(parent);
        } else {
            root = rbt_rr_rotate(parent);
        }
        return rbt_double_black(root, X);
    } 
    else if (!sibling || (sibling->left->color == BLACK && sibling->right->color == BLACK)) {
        if (sibling)
            sibling->color = RED;
        if (parent->color == BLACK)
            return rbt_double_black(root, parent);
        else {
            parent->color = BLACK;
            return root;
        }
    } 
    else {
        if (sibling->left && sibling->left->color == RED) {
            if (X == parent->left) {
                sibling->left->color = BLACK;
                root = rbt_rr_rotate(sibling);
            } 
            else {
                sibling->left->color = parent->color;
                root = rbt_rr_rotate(sibling);
                root = rbt_ll_rotate(parent);
            }
        } 
        else if (sibling->right && sibling->right->color == RED) {
            if (X == parent->right) {
                sibling->right->color = BLACK;
                root = rbt_ll_rotate(sibling);
            } 
            else {
                sibling->right->color = parent->color;
                root = rbt_ll_rotate(sibling);
                root = rbt_rr_rotate(parent);
            }
        }

        parent->color = BLACK;
        return root;
    }
}

Node *rbt_insert(Node *root, struct packet *pkt, size_t *size) {
    Node *Z = NULL;

    root = node_insert(root, pkt, &Z);
    if (Z) {
        *size += 1;
        rbt_balance_insert(Z);
    }

    while (root && root->parent) 
        root = root->parent;

    if (root)
        root->color = BLACK;

    return root;
}



Node *rbt_delete(Node *root, const unsigned short id) {
    if (!root)
        return NULL;
    if (id < root->id)
        root->left = rbt_delete(root->left, id);
    else if (root->id < id)
        root->right = rbt_delete(root->right, id);

    return node_delete(root, root);
}


Node *rbt_successor(Node *root) {
    if (!root->left)
        return root;
    return rbt_successor(root->left);
}


Node *rbt_restructure(Node *Z) {
    Node *X = rbt_grandparent(Z);


    if (X->left == Z->parent) {
        if (Z == Z->parent->left)
            X = rbt_ll_rotate(X);
        else
            X = rbt_lr_rotate(X);
    }
    else if (X->right == Z->parent) { 
        if (Z == Z->parent->left)
            X = rbt_rl_rotate(X);
        else
            X = rbt_rr_rotate(X);
    }
    
    X->color = BLACK;

    if (X->left)
        X->left->color = RED;

    if (X->right)
        X->right->color = RED;

    return X;
}


Node *rbt_recolor(Node *Z) {
    Node *X = rbt_grandparent(Z);
    Node *uncle = rbt_uncle(Z);

    Z->parent->color = BLACK;

    if (uncle)
        uncle->color = BLACK;

    if (X)
        X->color = RED;

    return X;
}


Node *rbt_balance_insert(Node *Z) {
    if (!Z || !Z->parent) {
        if (Z)
            Z->color = BLACK;
        return Z;
    }

    if (Z->color == RED && Z->parent->color == RED) {
        Node *uncle = rbt_uncle(Z);
        if (!uncle || uncle->color == BLACK) {
            Z = rbt_restructure(Z);
            return rbt_balance_insert(Z);
        }
        else {
            Z = rbt_recolor(Z);
            return rbt_balance_insert(rbt_grandparent(Z));
        }
    }

    return rbt_balance_insert(Z->parent);
}


Node *rbt_ll_rotate(Node *X) {
    Node *Y = X->left;
    Node *T = Y->right;

    X->left = T;
    Y->right = X;

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

    return Y;
}


Node *rbt_rr_rotate(Node *X) {
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

    return Y;
}


Node *rbt_lr_rotate(Node *X) {
    Node *Y = X->left;
    Node *Z = Y->right;

    Node *T1 = Z->left;
    Node *T2 = Z->right;

    Z->parent = X->parent;
    Z->left = Y;
    Z->right = X;

    Y->right = T1;
    Y->parent = Z;

    X->parent = Z;
    X->left = T2;

    if (T1)
        T1->parent = Y;
    if (T2)
        T2->parent = X;

    if (Z->parent) {
        if (Z->parent->left == X)
            Z->parent->left = Z;
        else 
            Z->parent->right = Z;
    }

    return Z;
}


Node *rbt_rl_rotate(Node *X) {
    Node *Y = X->right;
    Node *Z = Y->left;

    Node *T1 = Z->left;
    Node *T2 = Z->right;

    Z->parent = X->parent;
    Z->left = X;
    Z->right = Y;

    X->parent = Z;
    X->right = T1;

    Y->parent = Z;
    Y->left = T2;

    if (T1)
        T1->parent = X;
    if (T2)
        T2->parent = Y;

    if (Z->parent) {
        if (Z->parent->left == X)
            Z->parent->left = Z;
        else
            Z->parent->right = Z;
    }

    return Z;
}


void rbt_inorder(Node *root) {
    if (!root) {
        return;
    }

    rbt_inorder(root->left); 
    printf("%d ", root->id);
    rbt_inorder(root->right); 
}


Node *rbt_grandparent(Node *node) {
    if (!node || !node->parent)
        return NULL;

    return node->parent->parent;
}


Node *rbt_uncle(Node *node) {
    if (!node || !node->parent || !rbt_grandparent(node))
        return NULL;

    Node *gp = rbt_grandparent(node);
    return gp->left == node->parent ? gp->right : gp->left;
}


void rbt_print_tree(Node *root, size_t space) {
    if (root) {
        space += 10;

        rbt_print_tree(root->right, space);
        printf("\n");

        for (size_t i = 10; i < space; i++)
            printf(" ");

        printf("%d(%s)\n", root->id, root->color == RED ? "R" : "B");
        rbt_print_tree(root->left, space);
    }
}


unsigned short rbt_next(Node *root, unsigned short *expected) {
    if (!root)
        return (unsigned short)-1;

    unsigned short left = rbt_next(root->left, expected);

    if (left != (unsigned short)-1)
        return left;

    if (root->id != *expected)
        return *expected;

    (*expected)++;

    unsigned short right = rbt_next(root->right, expected);

    if (right != (unsigned short)-1)
        return right;

    return (unsigned short)-1;
}

Node *rbt_sibling(Node *node) {
    Node *parent = node->parent;
    if (parent)
        return parent->left == node ? parent->right : parent->left;
    return NULL;
}
