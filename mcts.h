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

    action::place get_move() {
        return parent_move;
    }

    double UCB(bool tuned = false) {
        if (tuned)  return Q + std::sqrt(std::log(parent->N) / N * std::min(0.25, mean - mean * mean));
        else        return Q + std::sqrt(std::log(parent->N) / N) * 0.1;
    }

    /**
     * return the child node with highest UCB, or itself if the node has unexplored child
     */
    node* select(bool tuned = false) {
        //std::cout<<"Selection started!      ";
        if (num_of_child != explored_child || num_of_child == 0)     return this;
        double best_value = 0;
        node* best=NULL;
        for (node* child : children) {
            double value = child->UCB(tuned);
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
            std::vector<action::place> space(board::size_x * board::size_y);
            for (size_t i = 0; i < space.size(); i++)
                space[i] = action::place(i, who);
            std::shuffle(space.begin(), space.end(), engine);
            for (const action::place& move : space) {
                board after = current;
                if (move.apply(after) == board::legal) {
                    node* child = new node(after, child_type);
                    child->parent = this;
                    child->parent_move = move;
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
        std::vector<int> space(board::size_x * board::size_y);
        board simulate = current;
        board::piece_type current_player = who;
        bool checkmate = true;
        for (size_t i = 0; i < sizeof(space); i++) {
            space[i] = i;
        }
        for (;;) {
            std::shuffle(space.begin(), space.end(), engine);
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
        action::place best_move;
        for (node* child : children) {
            int visit_count = child->get_visit_count();
            if (visit_count >= most_visit_count) {
                most_visit_count = visit_count;
                best_move = child->get_move();
            } 
        }
        return best_move;
    }

private:
    board current;
    int num_of_child = 0, explored_child = 0;
    board::piece_type who;  //type to play next
    board::piece_type child_type;
    node* parent = NULL;
    action::place parent_move;
    std::vector<node*> children;
    bool terminated = false;
    int N = 0;
    double Q = 0, mean = 0;
};

class mcts{
public:
    mcts(const board& root_board, board::piece_type player_type, int c, int t, bool tu = true) : 
        root(root_board, player_type), cycles(c), think_time(t), tuned(tu){}

    node* select(bool tuned = true) {
        node* selecting = &root;
        node* next;
        while ((next = selecting->select(tuned)) != selecting) {
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