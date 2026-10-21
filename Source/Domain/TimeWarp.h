#pragma once

#include <functional>
#include <string>
#include <tuple>
#include <vector>

// GateRunner's normalized, chunk-local timing model. Expressions are bounded
// and compiled once when preparing a pattern, never evaluated per sample.
namespace TimeWarp
{
struct Settings
{
    bool enabled = false;
    std::string curve = "lin";
    std::string expression = "T";
    int repeats = 1;
    double amount = 100.0;
    int quantize = 0; // Off, or 1/2/4/8 divisions per original step.
    bool noteLengths = true;

    bool operator==(const Settings& other) const
    {
        return std::tie(enabled, curve, expression, repeats, amount, quantize, noteLengths)
            == std::tie(other.enabled, other.curve, other.expression, other.repeats,
                        other.amount, other.quantize, other.noteLengths);
    }
};

struct Curve
{
    const char* name;
    const char* group;
    std::function<double(double)> function;
};

struct Resolution
{
    std::function<double(double)> function;
    std::string error;
};

const std::vector<Curve>& curves();
bool isKnownCurve(const std::string& name);
bool isQuantize(int value);
Resolution compileExpression(const std::string& source);
Resolution resolve(const Settings& settings);
double normalized(double t, const Resolution& curve, double amount);

struct Timing { double onset, duration; };
// All inputs/outputs are in original steps. minDuration is 0.5ms in steps.
Timing timing(double step, double duration, int length, const Settings& settings,
              const Resolution& curve, double minDuration);
}
