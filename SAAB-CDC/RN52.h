#ifndef RN52_H_
#define RN52_H_

class RN52handler
{
    
public:
    void initializeAtmelPins();
    void writeToUart(const char* c, int len);
};
#endif /* RN52_H_ */
