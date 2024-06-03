#include <queue>

float millis();

float predictTemp(std::queue<float> recordedTemps, std::queue<float> recordedTimes){
    float hypothesizedTemp;
    
    recordedTemps.pop();
    recordedTemps.push(hypothesizedTemp);
    recordedTimes.pop();
    recordedTimes.push(millis());

    return hypothesizedTemp;
}