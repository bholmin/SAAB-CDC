#ifndef RN52_H_
#define RN52_H_

class RN52Class
{
    
public:
    void initializeAtmelPins();
    void toUART(const char* c, int len);
    void fromUART();
    void fromSPP(const char* c, int len);
    void onGPIO2();
};
#endif /* RN52_H_ */
