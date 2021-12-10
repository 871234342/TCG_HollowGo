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

    board::piece_type get_who() {
        return who;
    }

    /**
     * return the child node with highest UCB, or itself if the node is leaf
     */
    node* select() {
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

    /**
     * return the node to simulate
     * if is leaf (num_of_child == 0), expand all children before return 
     */
    node* expand() {
        if (num_of_child == 0) {
            // generate all children
            std::vector<action::place> space(board::size_x * board::size_y);
            for (size_t i = 0; i < space.size(); i++)
                space[i] = action::place(i, who);
            std::default_random_engine rng;
            std::shuffle(space.begin(), space.end(), rng);
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
    bool simulate(board::piece_type root_type) {
        std::vector<int> space(board::size_x * board::size_y);
        std::default_random_engine rng;
        board simulate = current;
        bool my_turn = root_type == who;
        bool checkmate = true;
        for (size_t i = 0; i < sizeof(space); i++) {
            space[i] = i;
        }
        for (;;my_turn = !my_turn) {
            std::shuffle(space.begin(), space.end(), rng);
            for (int pos : space) {
                if (simulate.place(pos % 9, pos / 9) == board::nogo_move_result::legal) {
                    checkmate = false;
                    break;
                }
            }
            if (checkmate) {
                return my_turn;
            }
            else {
                checkmate = true;
            }
        }
    }

    /**
     * return the parent node to update
     */
    node* update(bool victory) {
        played++;
        if (victory)    win++;
        return parent;
    }

private:
    board current;
    int num_of_child, explored_child, win, played;
    board::piece_type who;  //type to play next
    board::piece_type child_type;
    node* parent = NULL;
    action::place parent_move;
    std::vector<node*> children;
};

class mcts{
public:
    mcts(board& root_board, board::piece_type player_type, int iter) : root(root_board, player_type){}

    node* select() {
        node* selecting = &root;
        node* next;
        while ((next = selecting->select()) != selecting) {
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



private:
    node root;
};