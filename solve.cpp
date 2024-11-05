#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <vector>

// row slide states: 9 (3 top * 3 bot) = 4 bits
// row values: 10 values, 10 slots = 40 bits (4 bits/value)
// plus 8 bits for distance at the top for sorting
// plus 8 bits for depth at the top for sorting

static inline long long put_value(long long state, std::size_t i, unsigned char value) {
    return state | (((long long) (value & 0x0F)) << (4 * i));
}

static inline long long put_distance(long long state, unsigned char value) {
    return state | (((long long) value) << (4 * 10));
}

static inline long long put_depth(long long state, unsigned char value) {
    return state | (((long long) value) << (4 * 12));
}

static inline int get_depth(long long state) {
    return (state >> 48) & 0xFF;
}

static inline long long strip_depth(long long state) {
    return state & 0x0000FFFFFFFFFFFF;
}

static inline int get_distance(long long state) {
    return (state >> 40) & 0xFF;
}

long long compress_puzzle_state(const unsigned char * uncompressed_state) {
    long long compressed_state = 0;
    for (auto i = 0; i < 12; ++i) {
        compressed_state = put_value(compressed_state, i, uncompressed_state[i]);
    }
    return compressed_state;
}

static const unsigned char COMPLETED_STATE_UNCOMPRESSED[] = {
    0, 1, 2, 3, 4,
    5, 6, 7, 8, 9,
    1, 1,
};

static const long long COMPLETED_STATE =
    compress_puzzle_state(COMPLETED_STATE_UNCOMPRESSED);

static const long long VALUE_MASK = 0x000000FFFFFFFFFF; 

bool is_complete(long long state) {
    return ((COMPLETED_STATE & VALUE_MASK) ^ (state & VALUE_MASK)) == 0;
}

static inline long long get_value(long long state) {
    return state & VALUE_MASK;
}

// X 2 2 4 4
// 1 1 1 3 3
static const int DIST_LOOKUP_0[] = {
    0, 2, 2, 4, 4,
    1, 1, 1, 3, 3,
};
// 2 X 2 2 4
// 1 1 1 1 3
static const int DIST_LOOKUP_1[] = {
    2, 0, 2, 2, 4,
    1, 1, 1, 1, 3,
};
// 2 2 X 2 2
// 1 1 1 1 1
static const int DIST_LOOKUP_2[] = {
    2, 2, 0, 2, 2,
    1, 1, 1, 1, 1,
};
// 4 2 2 X 2
// 3 1 1 1 1
static const int DIST_LOOKUP_3[] = {
    4, 2, 2, 0, 2,
    3, 1, 1, 1, 1,
};
// 4 4 2 2 X
// 3 3 1 1 1
static const int DIST_LOOKUP_4[] = {
    4, 4, 2, 2, 0,
    3, 3, 1, 1, 1,
};
// 1 1 1 3 3
// X 2 2 4 4
static const int DIST_LOOKUP_5[] = {
    1, 1, 1, 3, 3,
    0, 2, 2, 4, 4,
};
// 1 1 1 1 3
// 2 X 2 2 4
static const int DIST_LOOKUP_6[] = {
    1, 1, 1, 1, 3,
    2, 0, 2, 2, 4,
};
// 1 1 1 1 1
// 2 2 X 2 2
static const int DIST_LOOKUP_7[] = {
    1, 1, 1, 1, 1,
    2, 2, 0, 2, 2,
};
// 3 1 1 1 1
// 4 2 2 X 2
static const int DIST_LOOKUP_8[] = {
    3, 1, 1, 1, 1,
    4, 2, 2, 0, 2,
};
// 3 3 1 1 1
// 4 4 2 2 X
static const int DIST_LOOKUP_9[] = {
    3, 3, 1, 1, 1,
    4, 4, 2, 2, 0,
};

static const int * const DIST_LOOKUP[] = {
    DIST_LOOKUP_0,
    DIST_LOOKUP_1,
    DIST_LOOKUP_2,
    DIST_LOOKUP_3,
    DIST_LOOKUP_4,
    DIST_LOOKUP_5,
    DIST_LOOKUP_6,
    DIST_LOOKUP_7,
    DIST_LOOKUP_8,
    DIST_LOOKUP_9,
};

int tile_distance(std::size_t i, std::size_t target_i) {
    return DIST_LOOKUP[i][target_i];
}

static inline unsigned char value_at(long long state, std::size_t i) {
    return (state >> (4 * i)) & 0x0F;
}

// distance function
unsigned char distance_from_completion(long long state) {
    // ignore slider state
    auto total_d = 0;
    // std::cout << "getting distance for state " << state << " :";
    for (auto i = 0; i < 10; ++i) {
        auto target_i = value_at(state, i);
        auto d = tile_distance(i, target_i);
        // std::cout << " " << d;
        total_d += d;
    }
    // std::cout << " = " << total_d << std::endl;
    return total_d;
}

// 9 next states, given a state
// 9 lookups of index swaps
//   a b c d e    0 1 2 3 4
//   f g h i j    5 6 7 8 9
//
static const int INDEX_SWAPS[9][12] = {
    // - -  f g h  d e
    // - -  a b c  i j
    {       5,6,7, 3,4,
            0,1,2, 8,9,
       2,2,
    },

    // - -  g h i  d e
    // - f  a b c  j -
    {       6,7,8, 3,4,
         5, 0,1,2, 9,
       2,1,
    },

    // - -  h i j  d e
    // f g  a b c  - -
    {       7,8,9, 3,4,
       5,6, 0,1,2,
       2,0,
    },
    // - a  f g h  e -
    // - -  b c d  i j
    {    0, 5,6,7, 4,
            1,2,3, 8,9,
       1,2,
    },

    // - a  g h i  e -
    // - f  b c d  j -
    {    0, 6,7,8, 4,
         5, 1,2,3, 9,
       1,1,
    },

    // - a  h i j  e -
    // f g  b c d  - -
    {    0, 7,8,9, 4,
       5,6, 1,2,3,
       1,0,
    },

    // a b  f g h  - -
    // - -  c d e  i j
    {  0,1, 5,6,7,
            2,3,4, 8,9,
       0,2,
    },

    // a b  g h i  - -
    // - f  c d e  j -
    {  0,1, 6,7,8,
         5, 2,3,4, 9,
       0,1,
    },

    // a b  h i j  - -
    // f g  c d e  - -
    {  0,1, 7,8,9,
       5,6, 2,3,4,
       0,0,
    },
};

struct MovedState {
    long long state;
    unsigned char slider;
};

MovedState next_state(long long state, std::size_t swap_i) {
    const int * index_swap = INDEX_SWAPS[swap_i];
    long long next = 0;
    for (auto i = 0; i < 10; ++i) {
        auto value = value_at(state, i);
        auto target_i = index_swap[i];
        next = put_value(next, target_i, value);
    }
    // slider state post swap
    auto slider = ((index_swap[10] & 0x03) << 2) | (index_swap[11] & 0x03);
    // next = put_value(next, 10, slider_value);
    return MovedState{next, static_cast<unsigned char>(slider)};
}

struct Link {
    long long state;
    unsigned char slider;
    std::shared_ptr<const struct Link> prev;

    Link(long long s, unsigned char l, std::shared_ptr<const struct Link> p) : state(s), slider(l), prev(p) {}
};

void print_link(int count, const Link * link) {
    auto slider_top = (link->slider >> 2) & 0x03;
    if (count >= 0) {
        std::cout << count << ". [";
    } else {
        std::cout << "   [";
    }
    for (auto i = 0; i < slider_top; ++i) {
        std::cout << " -";
    }

    for (auto i = 0; i < 5; ++i) {
        std::cout << " " << static_cast<int>(value_at(link->state, i));
    }

    for (auto i = slider_top; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ]" << std::endl;

    auto slider_bot = link->slider & 0x03;
    std::cout << "   [";

    for (auto i = 0; i < slider_bot; ++i) {
        std::cout << " -";
    }

    for (auto i = 5; i < 10; ++i) {
        std::cout << " " << static_cast<int>(value_at(link->state, i));
    }

    for (auto i = slider_bot; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ]" << std::endl;
}

void print_flip(int count, const Link * from, const Link * to) {
    auto slider_top = (from->slider >> 2) & 0x03;
    std::cout << count << ". [";
    for (auto i = 0; i < slider_top; ++i) {
        std::cout << " -";
    }

    for (auto i = 0; i < 5; ++i) {
        std::cout << " " << static_cast<int>(value_at(from->state, i));
    }

    for (auto i = slider_top; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ]   ";

    // top to
    slider_top = (to->slider >> 2) & 0x03;
    std::cout << "/- [";
    for (auto i = 0; i < slider_top; ++i) {
        std::cout << " -";
    }

    for (auto i = 0; i < 5; ++i) {
        std::cout << " " << static_cast<int>(value_at(to->state, i));
    }

    for (auto i = slider_top; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ]" << std::endl;

    // bot from

    auto slider_bot = from->slider & 0x03;
    std::cout << "   [";

    for (auto i = 0; i < slider_bot; ++i) {
        std::cout << " -";
    }

    for (auto i = 5; i < 10; ++i) {
        std::cout << " " << static_cast<int>(value_at(from->state, i));
    }

    for (auto i = slider_bot; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ] -/";

    // bot to
    slider_bot = to->slider & 0x03;
    std::cout << "   [";

    for (auto i = 0; i < slider_bot; ++i) {
        std::cout << " -";
    }

    for (auto i = 5; i < 10; ++i) {
        std::cout << " " << static_cast<int>(value_at(to->state, i));
    }

    for (auto i = slider_bot; i < 2; ++i) {
        std::cout << " -";
    }
    std::cout << " ]" << std::endl;
}

int max_depth = -1;
int max_distance_at_depth = 0;
bool first = true;

template <typename T>
bool process(std::shared_ptr<const Link> cur_link, std::set<long long> & seen, std::priority_queue<std::shared_ptr<const Link>, std::vector<std::shared_ptr<const Link> >, T> & queue) {
    // TODO: strip depth out of state
    // auto comparable_state = strip_depth(cur_link->state);
    auto comparable_state = get_value(cur_link->state);
    if (seen.count(comparable_state) > 0) {
        // std::cout << "already seen: " << cur_link->state << std::endl;
        return false;
    }

    auto depth = get_depth(cur_link->state);
    auto dist = get_distance(cur_link->state);

    if (depth > max_depth) {
        max_depth = depth;
        max_distance_at_depth = dist;
        if (first) {
            first = false;
        } else {
            std::cout << std::endl;
        }
        std::cout << "searching depth = " << depth << ", dist = " << dist << "..." << std::flush;
    } else if (dist > max_distance_at_depth) {
        max_distance_at_depth = dist;
        std::cout << dist << "..." << std::flush;
    }

    seen.insert(comparable_state);

    if (is_complete(comparable_state)) {
        std::cout << "solution found!!!" << std::endl;
        const Link * link = cur_link.get();
        std::shared_ptr<const Link> rev_link = nullptr;
        int last_slider = -1;
        while (link != nullptr) {
            if (last_slider >= 0) {
                rev_link = std::make_shared<const Link>(link->state, last_slider, rev_link);
            }
            last_slider = link->slider;
            if (link->prev.get() != nullptr) {
                rev_link = std::make_shared<const Link>(link->state, link->slider, rev_link);
            }
            link = link->prev.get();
        }
        link = rev_link.get();
        int count = 1;
        while (link != nullptr) {
            print_flip(count, link, link->prev.get());
            link = link->prev.get()->prev.get();
            count += 1;
        }
        std::cout << "END solution" << std::endl;
        return true;
    }

    //long long next_states[9];
    for (auto i = 0; i < 9; ++i) {
        auto moved_state_i = next_state(comparable_state, i);
        auto next_state_i = moved_state_i.state;
        auto next_state_i_d = distance_from_completion(next_state_i);

        //std::cout << "next state " << i <<  ": " << next_state_i << " dist = " << ((int) next_state_i_d) << std::endl;
        next_state_i = put_distance(next_state_i, next_state_i_d);

        //std::cout << "dist aug state " << i <<  ": " << next_states[i] << std::endl;
    //}

    // sort (not necessary with queue)
    //std::sort(std::begin(next_states), std::end(next_states));

    // set up linked list link for cur_state
    //for (auto i = 0; i < 9; ++i) {
        // check if next_state_i has already been visited
        /*
        bool already_visited = false;
        auto ancestor = prev;
        while (ancestor != nullptr) {
            if (ancestor->state == next_states[i]) {
                already_visited = true;
                break;
            }
            ancestor = ancestor->prev;
        }
        if (already_visited) {
            std::cout << "already visited state " << next_states[i] << std::endl;
            continue;
        }
        */
        if (seen.count(next_state_i) == 0) {
            auto depth = 0;
            auto ancestor = cur_link.get();
            while (ancestor != nullptr) {
                depth += 1;
                ancestor = ancestor->prev.get();
            }

            // N.B. this is just for sorting, strip this out before comparing
            next_state_i = put_depth(next_state_i, depth);

            // std::cout << "already seen: " << cur_link->state << std::endl;
            auto next_link = std::make_shared<const Link>(next_state_i, moved_state_i.slider, cur_link);
            queue.push(next_link);
        }
        // recurse
        //if (rec_search(depths_allowed - 1, next_states[i], cur_link, seen, queue)) {
        //    return true;
        //}
    }
    return false;
}

void solve(const unsigned char * initial_puzzle_state_uncompressed) {
    long long state = compress_puzzle_state(initial_puzzle_state_uncompressed);
    //std::cout << "initial_state: " << state << std::endl;
    //std::cout << "target_state: " << COMPLETED_STATE << std::endl;
    //std::cout << "initial_state is_equal to target_state: " << is_complete(state) << std::endl;
    //std::cout << "target_state is_equal to itself: " << is_complete(COMPLETED_STATE) << std::endl;
    //std::cout << "initial_d: " << (int) distance_from_completion(state) << std::endl;
    //std::cout << "target_state distance to itself: " << (int) distance_from_completion(COMPLETED_STATE) << std::endl;

    //auto first_link = std::make_shared<const Link>(Link (state, nullptr) );
    auto first_link = std::make_shared<const Link>(state, 0x05, nullptr);

    print_link(0, first_link.get());

    std::set<long long> seen;

    auto cmp = [](std::shared_ptr<const Link> left, std::shared_ptr<const Link> right) { 
       return (left->state) > (right->state);
    };

    std::priority_queue<decltype(first_link), std::vector<decltype(first_link)>, decltype(cmp)> queue(cmp);
    queue.push(first_link);

    while (true) {
        if (queue.empty()) {
            std::cout << "queue is empty" << std::endl;
            break;
        }
        auto link = queue.top();
        queue.pop();
        if (process(link, seen, queue)) {
            break;
        }
    }
}

bool parse_input(const std::string& initial_puzzle_input, uint8_t (&initial_puzzle_state_uncompressed)[12]) {
    if (initial_puzzle_input.empty()) {
        return true; // use the default...
    }
    if (initial_puzzle_input.size() != 11) {
        std::cout << "Parse error: Invalid format: Expecting xxxxx-xxxxx" << std::endl;
        return false;
    }
    size_t i = 0;
    bool has_value[10] = {false,};
    for (char c : initial_puzzle_input) {
        if (c == '-' && i == 5) {
            continue;
        }
        if (c >= '0' && c <= '9') {
            uint8_t value = static_cast<uint8_t>(c) - 48;
            if (has_value[value]) {
                std::cout << "Parse error: " << static_cast<int>(value) << " has appeared twice" << std::endl;
                return false;
            }
            has_value[value] = true;
            initial_puzzle_state_uncompressed[i] = static_cast<uint8_t>(c) - 48;
            ++i;
        }
        else
        {
            return false;
        }
    }
    if (i != 10) {
        return false;
    }
    for (bool has_value_i : has_value) {
        if (!has_value_i) {
            return false;
        }
    }

    // start with puzzle centered
    initial_puzzle_state_uncompressed[10] = 1;
    initial_puzzle_state_uncompressed[11] = 2;
    return true;
}

int main() {
    unsigned char initial_puzzle_state_uncompressed[] = {
        1, 2, 0, 6, 7,
        5, 9, 3, 4, 8,
        1, 1,
    };

    std::string initial_puzzle_input;
    std::cout << "Initial puzzle [12067-59348]: ";
    std::getline(std::cin, initial_puzzle_input);
    std::cout << std::endl;

    if (!parse_input(initial_puzzle_input, initial_puzzle_state_uncompressed)) {
        return 1;
    }

    solve(initial_puzzle_state_uncompressed);

    // TODO: go out from solution to depth X
    return 0;
}