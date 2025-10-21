#include <iostream>
#include <cstdint>

using namespace std;

enum class ALUOp
{
    ADD, SUB, AND, OR, XOR, NOT, INC, DEC, PASS
};

struct ALUResult
{
    uint16_t result;
    bool zero;
    bool negative;
    bool overflow;
    bool carry;
};

ALUResult alu (uint16_t a, uint16_t b, ALUOp op)
{
    ALUResult res {};
    uint32_t temp = 0;

    switch (op)
    {
        case ALUOp::ADD:
            temp = static_cast<uint32_t>(a) + b;
            res.result = static_cast<uint16_t>(temp);
            res.carry = temp > 0xFFFF;
            res.overflow = ((~(a^b)) & (a ^ res.result)) & 0x8000;
            break;

        case ALUOp::SUB:
            temp = static_cast<uint32_t>(a) - b;
            res.result = static_cast<uint16_t>(temp);
            res.carry = a >= b;
            res.overflow = ((a^b) & (a ^ res.result)) & 0x8000;
            break;

        case ALUOp::AND:
            res.result = a & b;
            break;

        case ALUOp::OR:
              res.result = a | b;
            break;

        case ALUOp::XOR:
                res.result = a ^ b;
                break;

        case ALUOp::NOT:
                res.result = ~a;
                break;

        case ALUOp::INC:
            res.result = a + 1;
            res.carry = (a == 0xFFFF);
            break;

        case ALUOp::DEC:
            res.result = a - 1;
            res.carry = (a == 0x0000);
            break;

        case ALUOp::PASS:
           res.result = a;
            break;
    }

    res.zero = (res.result == 0);
    res.negative = (res.result & 0x8000) != 0;

    return res;
}

int main()
{
   uint16_t a;
   uint16_t b;
   cout << "Input two 16-bits hex numbers: ";
   cin >> hex >> a >> b;

   ALUResult result = alu (a, b, ALUOp::ADD);
   cout << "ADD Result: 0x" << hex << result.result << endl;
   cout << "Zero: " << result.zero << ", Negative: " << result.negative << ", Carry: " << result.carry << ", Overflow: " << result.overflow << endl;
    
   result = alu (a, b, ALUOp::SUB);
   cout << "SUB Result: 0x" << hex << result.result << endl;

   result = alu (a, 0, ALUOp::NOT);
   cout << "NOT Result: 0x" << hex << result.result << endl;

   result = alu (a, 0, ALUOp::INC);
   cout << "INC Result: 0x" << hex << result.result << endl;

   result = alu (a, b, ALUOp::OR);
   cout << "OR Result: 0x" << hex << result.result << endl;

return 0;
}

