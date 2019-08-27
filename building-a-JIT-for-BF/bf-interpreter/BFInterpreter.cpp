#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

constexpr int MEMORY_SIZE = 30000;

class BFInterpreter
{
public:
    BFInterpreter() : pc_(0), data_ptr_(0), memory_(MEMORY_SIZE, 0) {}
    BFInterpreter(const BFInterpreter&) = delete;
    BFInterpreter(BFInterpreter&&) = delete;
    void interp(std::istream& stream);

private:
    std::string instructions_;
    std::unordered_map<size_t, size_t> jump_table_;
    size_t pc_;
    size_t data_ptr_;
    std::vector<uint8_t> memory_;
};

void BFInterpreter::interp(std::istream& stream)
{
    pc_ = 0;
    data_ptr_ = 0;
    std::vector<size_t> stack;
    for (std::string line; std::getline(stream, line);)
    {
        for (auto c : line)
        {
            if (c == '>' || c == '<' || c == '+' || c == '-' || c == '.' || c == ',' ||
                c == '[' || c == ']')
            {
                instructions_.push_back(c);
            }

            if (c == '[')
            {
                stack.push_back(instructions_.size() - 1);
            }
            if (c == ']')
            {
                size_t back = stack.back();
                stack.pop_back();
                jump_table_.emplace(std::make_pair(back, instructions_.size() - 1));
                jump_table_.emplace(std::make_pair(instructions_.size() - 1, back));
            }
        }
    }
    assert(stack.empty());

    while (pc_ < instructions_.size())
    {
        char instruction = instructions_[pc_];
        switch (instruction)
        {
        case '>':
            data_ptr_++;
            break;
        case '<':
            data_ptr_--;
            break;
        case '+':
            memory_[data_ptr_]++;
            break;
        case '-':
            memory_[data_ptr_]--;
            break;
        case '.':
            std::cout.put(memory_[data_ptr_]);
            break;
        case ',':
            memory_[data_ptr_] = std::cin.get();
            break;
        case '[':
            if (memory_[data_ptr_] == 0)
            {
                pc_ = jump_table_[pc_];
            }
            break;
        case ']':
            if (memory_[data_ptr_] != 0)
            {
                pc_ = jump_table_[pc_] - 1;
            }
            break;
        default:
            assert(0 && "unreachable");
        }
        pc_++;
    }
}

int main(int argc, const char** argv)
{
    if (argc == 2)
    {
        std::ifstream file(argv[1]);
        BFInterpreter{}.interp(file);
    }
    else
    {
        std::cout << "usage: bf-interpteter helloworld.bf\n";
    }

    return 0;
}