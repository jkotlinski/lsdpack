#include <vector>

class Rule {
    public:
        virtual int width() = 0;
        virtual void transform(std::vector<unsigned int>* bytes) = 0;
}
