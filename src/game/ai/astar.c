#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "collections/dlist.h"

typedef struct Node {
    
    int x, y;
    int g, h, f;
    struct Node *parent;

} Node;

// A* pathfinding structure
typedef struct Astar {



} Astar;

int size;
int diagonal_movement_cost;
time_t run_time;
double kvalue;
Node *start, *end, *par;
bool diagonal, running, no_path, complete, trigger;

// TODO: may bge make borders an array? as we already know what is in each tile?
DoubleList *borders, *open, *closed, *path;

int min_width, min_height;
int max_width, max_height;

static int astar_search_border (int search_x, int search_y) {

    int retval = -1;

    Node *node = NULL;
    unsigned int i = 0;
    for (ListElement *le = dlist_start (borders); le; le = le->next) {
        node = (Node *) le->data;
        if (node->x == search_x && node->y == search_y) {
            retval = i;
            break;
        }

        i++;
    }

    return retval;

}

static void astar_calculate_node_values (int possible_x, int possible_y, Node *open_node, Node *parent) {

    // check for bounds
    if (possible_x < min_width || possible_y < min_height || possible_x > max_width || possible_x > max_height)
        return;

    // if the node is already a border node or a closed one or an open one,
    // don't make open node
    // if (astar_search_border (possible_x, possible_y) != -1 || astar_search_border (possible_x, possible_y))

}

void astar_find_path (Node *parent) {

    Node *open_node = NULL;
    if (diagonal) {
        // detects and adds one step of nodes to open list
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (i == 1 && j == 1) continue;
                int possible_x = (parent->x - size) + (size * i);
                int possible_y = (parent->y - size) + (size * j);

                int cross_border_x = parent->x + (possible_x - parent->x);
                int cross_border_y = parent->y + (possible_y - parent->y);

                // disables ability to cut corners around borders
                // if (astar_search_border (cross_border_x, parent->y))
            }
        }
    }

}

void astar_start (Node *s, Node *e) {

    start = s;
    start->g = 0;
    end = e;

    dlist_insert_after (closed, dlist_end (closed), start);

    time_t start_time = time (NULL);

}