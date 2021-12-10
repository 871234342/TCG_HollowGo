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

class node{
public:
    node(board new_board, board::piece_type player_type){
        // init
        who = player_type;
        if (who == board::black)    child_type = board::white;
        else                        child_type = board::black;
        current = new_board;
        num_of_child = 0;
        explored_child = 0;
    }

    node* select() {
        // return the chid node with highest UCB, or itself if the node is leaf
        if (num_of_child != explored_child || num_of_child == 0)     return this;
        double best_UCB = 0;
        double c = 0.1;  // a constant
        node* best;
        for (node* child : children) {
            double UCB = (double)child->win / (double)child->played + c * sqrt(log10(this->played) / (double)child->played);
            if (UCB >= best_UCB) {
                best_UCB = UCB;
                best = child;
            }
        }
        return best;
    }

    node* expand() {
        // expand a node, and return the node to simulate
        // if is leaf, create all children and do simulation on one of it
        // else do simulation on any child
        if (num_of_child == 0) {
            // generate all children
            std::vector<action::place> space(board::size_x * board::size_y);
            is_leaf = false;
            for (size_t i = 0; i < space.size(); i++)
                space[i] = action::place(i, who);
            std::default_random_engine rng;
            std::shuffle(space.begin(), space.end(), rng);
            for (const action::place& move : space) {
                board after = current;
                if (move.apply(after) == board::legal) {
                    node* child = new node(after, child_type);
                    child->parent = this;
                }
            }
            num_of_child = children.size();

            return children[explored_child++];
        }
        else {
            return children[explored_child++];
        }
    }

    bool simulate() {
        // return true if win, false if lose
        std::vector<int> space(board::size_x * board::size_y);
        std::default_random_engine rng;
        board simulate = current;
        bool opponent_turn = false;
        bool checkmate = true;
        for (size_t i = 0; i < sizeof(space); i++) {
            space[i] = i;
        }
        for (;;opponent_turn = !opponent_turn) {
            std::shuffle(space.begin(), space.end(), rng);
            for (int pos : space) {
                if (simulate.place(pos % 9, pos / 9) == board::nogo_move_result::legal) {
                    checkmate = false;
                    break;
                }
            }
            if (checkmate) {
                return opponent_turn;
            }
            else {
                checkmate = true;
            }
        }
    }

private:
    board current;
    int num_of_child, explored_child, win, played;
    board::piece_type who;
    board::piece_type child_type;
    bool is_leaf = true;
    node* parent = NULL;
    std::vector<node*> children;
};

class mcts{
public:
    mcts(board& root_board, board::piece_type player_type, int iter) : root(root_board, player_type){
        iteration = iter;
    }

    void select() {
        node* selecting = &root;
        if (selecting->select())
    }

private:
    node root;
    int iteration;
};