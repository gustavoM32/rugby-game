// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Internal headers
#include "direction.h"
#include "position.h"
#include "spy.h"

// Main header
#include "attacker.h"

// Macros
#define UNUSED(x) (void)(x) // Auxiliary to avoid error of unused parameter
// The actual size of the game is not known, so I'll assume this value will be
// enough to make the calculations
#define WEIGHT_DIMENSIONS 100

/*----------------------------------------------------------------------------*/
/*                          PRIVATE FUNCTIONS HEADERS                         */
/*----------------------------------------------------------------------------*/

static void init_barriers(int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS]);
static int max(int a, int b);
static int get_distance(position_t pos1, position_t pos2);
static direction_t move_weight_to_direction(int i, int j);
static bool is_barrier_at_position(int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS], position_t position);


static position_t get_guessed_enemy_position(position_t attacker_position);
static int calculate_enemy_proximity(position_t enemy_position, position_t position);
static int get_turns_to_change_priority();

static void init_move_weights(int move_weights[3][3]);
static void update_weights_by_barrier_existence(int move_weights[3][3],
  position_t central_position, int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS]);
static void update_weights_by_enemy_proximity(int move_weights[3][3],
  position_t central_position, position_t enemy_position);
static void update_weights_by_goal_distance(int move_weights[3][3]);
static void update_weights_by_vertical_priority(int move_weights[3][3],
  bool priority_is_up);
static direction_t get_move_direction_from_weights(int move_weights[3][3]);

/*----------------------------------------------------------------------------*/
/*                             PRIVATE FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

/* auxiliary functions */

void init_barriers(int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS]) {
  for (int i = 0; i < WEIGHT_DIMENSIONS; i++) {
    for (int j = 0; j < WEIGHT_DIMENSIONS; j++) {
      if (i == 0 || j == 0) barriers[i][j] = 1;
      else barriers[i][j] = 0;
    }
  }
}

int max(int a, int b) {
  if (a > b) return a;
  return b;
}

int get_distance(position_t pos1, position_t pos2) {
  return max(abs(pos1.i - pos2.i), abs(pos1.j - pos2.j));
}

direction_t move_weight_to_direction(int i, int j) {
  return (direction_t) {i - 1, j - 1};
}

bool is_barrier_at_position(int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS], position_t position) {
  return barriers[position.i][position.j];
}

/* strategy functions */

// guess the enemy initial position, it will be the same i coordinate but in the
// right of the weight grid;
position_t get_guessed_enemy_position(position_t attacker_position) {
  position_t enemy_position = attacker_position;
  enemy_position.i = WEIGHT_DIMENSIONS - 2; // the weight grid also has a border
  return enemy_position;
}

// this will ignore the barries, as taking them into account would requires structures
// that are probably beyond the scope of this assignment (queue, bfs, ...)
int calculate_enemy_proximity(position_t enemy_position, position_t position) {
  return 1 + get_distance(enemy_position, position);
}

// number of turns to change the vertical priority
int get_turns_to_change_priority() {
  return 5 + rand() % 10; // strategy paremeter
}

void init_move_weights(int move_weights[3][3]) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      move_weights[i][j] = 1;
    }
  }
}

void update_weights_by_barrier_existence(int move_weights[3][3],
  position_t central_position, int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS]) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (move_weights[i][j] == 0) continue;
      direction_t direction = move_weight_to_direction(i, j);
      position_t position = move_position(central_position, direction);
      if (is_barrier_at_position(barriers, position)) {
        move_weights[i][j] = 0;
      }
    }
  }
}

void update_weights_by_enemy_proximity(int move_weights[3][3],
  position_t central_position, position_t enemy_position) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (move_weights[i][j] == 0) continue;
      direction_t direction = move_weight_to_direction(i, j);
      position_t position = move_position(central_position, direction);
      move_weights[i][j] *= calculate_enemy_proximity(enemy_position, position);
    }
  }
}

void update_weights_by_goal_distance(int move_weights[3][3]) {
  static int goal_weights[3] = {1, 2, 17}; // strategy parameter
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (move_weights[i][j] == 0) continue;
      move_weights[i][j] *= goal_weights[j];
    }
  }
}

void update_weights_by_vertical_priority(int move_weights[3][3],
    bool priority_is_up) {
  static int vertical_weights[3] = {1, 4, 15}; // strategy parameter
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      if (move_weights[i][j] == 0) continue;
      int index = i;
      if (priority_is_up) index = 2 - index;
      move_weights[i][j] *= vertical_weights[index];
    }
  }
}

direction_t get_move_direction_from_weights(int move_weights[3][3]) {
  int sum = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      sum += move_weights[i][j];
    }
  }
  int chosen_value = rand() % sum;
  sum = 0;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      int start = sum;
      sum += move_weights[i][j];
      int end = sum;
      if (chosen_value >= start && chosen_value < end) {
        return move_weight_to_direction(i, j);
      }
    }
  }

  // this should never happen
  return (direction_t) DIR_RIGHT;
}

/*----------------------------------------------------------------------------*/
/*                              PUBLIC FUNCTIONS                              */
/*----------------------------------------------------------------------------*/

direction_t execute_attacker_strategy(
    position_t attacker_position, Spy defender_spy) {
  // static variable definitions
  static bool structures_initialized = false;
  static int number_of_moves = 0;
  static int moves_to_spy;
  static int barriers[WEIGHT_DIMENSIONS][WEIGHT_DIMENSIONS];

  static bool vertical_priority_is_up;
  static int moves_current_vertical_priority;
  static int moves_to_change_vertical_priority;

  static position_t attacker_initial_position;
  static position_t attacker_intended_position = INVALID_POSITION;
  static position_t enemy_position; // guessed position

  int move_weights[3][3];
  direction_t move_direction;
  bool has_spied = get_spy_number_uses(defender_spy) == 1;
  
  // initialize static variables only in the first call of the function
  if (!structures_initialized) {
    structures_initialized = true;
    int seed = time(NULL);
    srand(seed);
    // printf("seed = %d\n", seed);
    moves_to_spy = 3 + rand() % 5; // strategy parameter
    init_barriers(barriers);

    vertical_priority_is_up = rand() % 2;
    moves_current_vertical_priority = 0;
    moves_to_change_vertical_priority = get_turns_to_change_priority();

    attacker_initial_position = attacker_position;
    enemy_position = get_guessed_enemy_position(attacker_position);
  } else { // only after the first call
    // register barrier
    if (!equal_positions(attacker_position, attacker_intended_position)) {
      barriers[attacker_intended_position.i][attacker_intended_position.j] = 1;
    }
  }

  // spying decision
  if (!has_spied && number_of_moves >= moves_to_spy) { // spy enemy position
    enemy_position = get_spy_position(defender_spy);
    if (enemy_position.i > attacker_initial_position.i) { // enemy is below attacker's initial position
      vertical_priority_is_up = 1; // prioritize the upper area of the board
    } else {
      vertical_priority_is_up = 0;
    }
    moves_current_vertical_priority = 0;
    moves_to_change_vertical_priority = get_turns_to_change_priority();
  } else { // will not spy
    if (moves_current_vertical_priority >= moves_to_change_vertical_priority) {
      if (rand() % 3 > 0) { // strategy paremeter
        vertical_priority_is_up = !vertical_priority_is_up;
      }
      moves_current_vertical_priority = 0;
      moves_to_change_vertical_priority = get_turns_to_change_priority();
    }
  }

  // calculate move weights
  init_move_weights(move_weights);
  update_weights_by_barrier_existence(move_weights, attacker_position, barriers);
  update_weights_by_enemy_proximity(move_weights, attacker_position, enemy_position);
  update_weights_by_goal_distance(move_weights);
  update_weights_by_vertical_priority(move_weights, vertical_priority_is_up);

  // update variables related to move
  move_direction = get_move_direction_from_weights(move_weights);
  attacker_intended_position = move_position(attacker_position, move_direction);
  number_of_moves++;
  moves_current_vertical_priority++;

  return move_direction;
}

/*----------------------------------------------------------------------------*/
