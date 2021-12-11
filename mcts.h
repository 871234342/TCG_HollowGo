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

std::default_random_engine engine;

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
        return played;
    }

    action::place get_move() {
        return parent_move;
    }

    double UBC(int parent_played) {
        double log_parent_played = std::log(parent_played);
        double explore = std::sqrt(log_parent_played / played);
        double exploit = (double)win / (double)played;
        // std::cout<<exploit<<"+"<<explore<<"  \n";
        // if (exploit > 1) {
        //     char t;
        //     std::cout<<"HMM?: "<<win<<"/"<<played<<'\n';
        //     std::cin>>t;
        // }
        return explore + exploit;
    }

    /**
     * return the child node with highest UCB, or itself if the node has unexplored child
     */
    node* select() {
        //std::cout<<"Selection started!      ";
        if (num_of_child != explored_child || num_of_child == 0)     return this;
        double best_value = 0;
        node* best=NULL;
        //std::cout<<"Iterating...\n";
        for (node* child : children) {
            double value = child->UBC(played);
            if (value >= best_value) {
                best_value = value;
                best = child;
            }
        }
        //std::cout<<"  done with best_score="<<best_value<<'\n';
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
        played++;
        if (victory)    win++;
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
    int num_of_child = 0, explored_child = 0, win = 0, played = 0;
    board::piece_type who;  //type to play next
    board::piece_type child_type;
    node* parent = NULL;
    action::place parent_move;
    std::vector<node*> children;
    bool terminated = false;
};

class mcts{
public:
    mcts(const board& root_board, board::piece_type player_type) : root(root_board, player_type){}

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

    action::place tree_search(int cycles, bool debug = false) {
        char t;
        for (int i = 0; i < cycles; i++) {
            if (debug) {
                //std::cout << "selecting..\n";
                //std::cin >> t;
            }
            node* working = select();
            if (debug) {
                //std::cout << "expanding..\n";
                //std::cin >> t;
            }
            working = expand(working);
            if (debug) {
                //std::cout << "simulating..\n";
                //std::cin >> t;
            }
            bool result = simulate(working);
            if (debug) {
                //std::cout << "updating..\n";
                //std::cin >> t;
            }
            update(working, result);
        }
        return root.best_action();
    }

private:
    node root;
};