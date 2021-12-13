#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include <fstream>
#include <cmath>
#include <unistd.h>
#include <signal.h>

std::default_random_engine engine;

bool time_up = false;

void mcts_timeout(int sig) {
    time_up = true;
}

int space[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
9, 10, 11, 12, 13, 14, 15, 16, 17,
18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35,
36, 37, 38, 39, 40, 41, 42, 43, 44,
45, 46, 47, 48, 49, 50, 51, 52, 53,
54, 55, 56, 57, 58, 59, 60, 61, 62,
63, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 80};

class node{
public:
    node(board new_board, board::piece_type player_type){
        // init
        who = player_type;
        if (who == board::black)    child_type = board::white;
        else                        child_type = board::black;
        current = new_board;
    }

    ~node() {
        for (node* child : children) {
            delete child;
        }
    }

    board::piece_type get_who() {
        return who;
    }

    int get_visit_count() {
        return N;
    }

    int get_move() {
        return parent_move;
    }

    double UCB(bool tuned = false) {
        if (tuned)  return Q + std::sqrt(std::log(parent->N) / N * std::min(0.25, mean - mean * mean));
        else        return Q + std::sqrt(std::log(parent->N) / N) * 0.1;
    }

    /**
     * return the child node with highest UCB, or itself if the node has unexplored child
     */
    node* select(board::piece_type root_type, bool tuned = false) {
        //std::cout<<"Selection started!      ";
        if (num_of_child != explored_child || num_of_child == 0)     return this;
        double best_value = 0;
        double c = 0.7;
        node* best=NULL;
        for (node* child : children) {
            double value;
            if (root_type == who)   value = child->Q + std::sqrt(std::log(N) / child->N) * c;
            else                    value = (1 - child->Q) + std::sqrt(std::log(N) / child->N) * c;
            if (value >= best_value) {
                best_value = value;
                best = child;
            }
        }            
        return best;
    }

    /**
     * return the node expanded to simulate
     * if is leaf (num_of_child == 0), create all children first before return
     * if the node is terminal, return itself
     */
    node* expand() {
        if (num_of_child != 0 && explored_child == num_of_child) {
            std::cout<<"WTF?";
            exit(1);
        }
        if (terminated) {
            return this;
        }
        else if (num_of_child == 0) {
            // generate all children
            for (int pos : space) {
                board after = current;
                action::place move = action::place(pos, who);
                if (move.apply(after) == board::legal) {
                    node* child = new node(after, child_type);
                    child->parent = this;
                    child->parent_move = pos;
                    children.push_back(child);
                }
            }
            num_of_child = children.size();

            if (num_of_child == 0) {
                terminated = true;
                return this;
            }
            return children[explored_child++];
        }
        else {
            return children[explored_child++];
        }
    }

    /**
     * return true if win for root peice type
     * both player play randomly
     */ 
    bool simulate(board::piece_type root_player) {
        board simulate = current;
        board::piece_type current_player = who;
        bool checkmate = true;
        for (;;) {
            std::shuffle(&space[0], &space[81], engine);
            for (int pos : space) {
                action::place move = action::place(pos, current_player);
                if (move.apply(simulate) == board::legal) {
                    checkmate = false;
                    break;
                }
            }
            if (checkmate) {
                return current_player != root_player;
            }
            else {
                checkmate = true;
                current_player = (current_player == board::black ? board::white : board::black);
            }
        }
    }

    /**
     * return the parent node to update
     */
    node* update(bool victory) {
        mean = (mean * N + victory) / (N + 1);
        N++;
        Q += (victory - Q) / N;
        return parent;
    }

    action::place best_action() {
        int most_visit_count = 0;
        int best_move = -1;
        for (node* child : children) {
            int visit_count = child->get_visit_count();
            if (visit_count >= most_visit_count) {
                most_visit_count = visit_count;
                best_move = child->get_move();
            } 
        }
        return action::place(best_move, who);
    }

private:
    board current;
    int num_of_child = 0, explored_child = 0;
    board::piece_type who;  //type to play next
    board::piece_type child_type;
    node* parent = NULL;
    std::vector<node*> children;
    bool terminated = false;
    int N = 0, parent_move = -1;
    double Q = 0, mean = 0;
};

class mcts{
public:
    mcts(const board& root_board, board::piece_type player_type, int c, int t, bool tu = true) : 
        root(root_board, player_type), cycles(c), think_time(t), tuned(tu){}

    node* select(bool tuned = true) {
        node* selecting = &root;
        node* next;
        while ((next = selecting->select(root.get_who(), tuned)) != selecting) {
            selecting = next;
        }
        return selecting;
    }

    node* expand(node* to_expand) {
        return to_expand->expand();
    }

    bool simulate(node* to_simulate) {
        return to_simulate->simulate(root.get_who());
    }

    void update(node* leaf, bool win) {
        while((leaf = leaf->update(win)) != NULL) {
            continue;
        }
    }

    action::place tree_search(bool debug = false) {
        if (cycles != 0) {
            for (int i = 0; i < cycles; i++) {
                node* working = select();
                working = expand(working);
                bool result = simulate(working);
                update(working, result);
            }            
        }
        else {
            time_up = false;
            signal(SIGALRM, &mcts_timeout);
            ualarm(think_time * 1000, 0);
            while(1) {
                node* working = select();
                working = expand(working);
                bool result = simulate(working);
                update(working, result);
                if (time_up)   break;          
            }
        }
        return root.best_action();
    }

private:
    node root;
    int cycles;     // number of simulations
    int think_time; // thinking_time in milisecond;
    bool tuned = true;
};