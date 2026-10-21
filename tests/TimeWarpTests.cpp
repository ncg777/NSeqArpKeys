#include "../Source/Domain/TimeWarp.h"
#include "../Source/Domain/AssignmentState.h"
#include "../Source/Domain/PatternBankFile.h"
#include "TimeWarpCurveFixtures.h"
#include <cmath>
#include <stdexcept>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}
bool near(double a, double b) { return std::abs(a - b) < 1.0e-10; }
}

void testTimeWarp()
{
    using namespace TimeWarp;
    check(curves().size() == warpFixtures.size(), "GateRunner curve library is incomplete");
    for (const auto& fixture : warpFixtures)
    {
        Settings settings; settings.curve = fixture.name;
        const auto resolution = resolve(settings);
        check(resolution.error.empty(), "GateRunner curve is missing");
        for (size_t i = 0; i < fixture.values.size(); ++i)
            check(near(resolution.function(warpFixtureTimes[i]), fixture.values[i]), "Curve differs from GateRunner reference");
    }
    for (const auto& test : std::vector<std::pair<std::string, double>> {
        {"Y = T * 2 + sin(PI / 2)", 1.5}, {"-2^2", 4}, {"2^3^2", 512},
        {"1 / 0", 0}, {"1 % 0", 0}, {"sin()", 0}, {"cos()", 1}, {"pow()", 1},
        {"integer(-1.75)", -1}, {"y=T+E", 0.25 + std::exp(1.0)},
        {"max(1,2,3)", 3}, {"sin(0,99)", 0}, {"sin(0,sqrt(-1))", 0},
        {"saw_wform(T)", -0.5}, {"revsaw_wform(T)", 0.5},
        {"wform(T,0,.5,1)", 0.5}, {"stowform(T,seq(0,2,4,2,0))", 0.5} })
    {
        const auto expression = compileExpression(test.first);
        check(expression.error.empty() && near(expression.function(0.25), test.second), "Custom expression compatibility failed");
    }
    for (const auto* expression : { "sqrt(-1)", "1e308*1e308", "stowform(T,1)", "seq(T)+1", "seq(1,2)", "min()" })
    {
        const auto result = compileExpression(expression);
        check(result.error.empty() && result.function(0.25) == 0.25, "Nonfinite or nonscalar expression must use identity");
    }
    for (const auto& expression : std::vector<std::string> { "", "gain*T", "t", "tanh(T)", "T.foo", "constructor(T)",
            "sin(", "T T", "T=0", std::string(513, 'T'), std::string(34, '(') + "T" + std::string(34, ')'),
            std::string(34, '-') + "T", std::string(33, 'a') })
    {
        const auto result = compileExpression(expression);
        check(!result.error.empty() && result.function(0.25) == 0.25, "Invalid syntax or parser limit did not use identity");
    }
    Settings settings;
    settings.enabled = true; settings.curve = "custom"; settings.expression = "T^2";
    auto curve = resolve(settings);
    auto event = timing(1, 1, 4, settings, curve, 0.0005);
    check(near(event.onset, 0.25) && near(event.duration, 0.75), "Warped note endpoints incorrect");
    settings.noteLengths = false;
    event = timing(1, 1.5, 4, settings, curve, 0.0005);
    check(near(event.onset, 0.25) && near(event.duration, 1.5), "Onsets-only warp changed note length");
    settings.repeats = 2;
    event = timing(3, 1, 4, settings, curve, 0.0005);
    check(near(event.onset, 2.5), "Repeats must warp inside each chunk");
    settings.quantize = 1;
    event = timing(3, 1, 4, settings, curve, 0.0005);
    check(near(event.onset, 3), "Quantization must use original steps within chunks");
    settings.amount = 50; settings.repeats = 1; settings.quantize = 0;
    event = timing(1, 1, 4, settings, curve, 0.0005);
    check(near(event.onset, 0.625), "Warp amount did not blend with original time");
    settings.amount = 0; settings.quantize = 8;
    event = timing(1, 1, 4, settings, curve, 0.0005);
    check(event.onset == 1 && event.duration == 1, "Zero amount must bypass warp and quantize");
    settings.amount = 100; settings.curve = "invlin"; settings.noteLengths = true;
    event = timing(1, 1, 4, settings, resolve(settings), 0.0005);
    check(near(event.onset, 3) && near(event.duration, 1), "Folded/reversed durations must be positive");
    settings.curve = "custom"; settings.expression = "2";
    event = timing(1, 1, 4, settings, resolve(settings), 0.0005);
    check(event.onset == 4 && event.duration == 0.0005, "Clamped coincident endpoints need a minimum duration");

    KeyAssignment assignment; assignment.timeWarp = settings;
    assignment.timeWarp.expression = "T + sin(2*PI*T)/8";
    assignment.timeWarp.repeats = 7; assignment.timeWarp.quantize = 4; assignment.timeWarp.amount = 73;
    juce::XmlElement xml("Assignment"); AssignmentState::write(xml, assignment);
    check(AssignmentState::read(xml) == assignment, "Warp settings lost in portable assignment");
    check(PatternBankFile::invalidAssignmentAttribute(xml).isEmpty(), "Valid warp was rejected by bank validation");
    for (const auto& field : std::vector<std::pair<const char*, const char*>> {
        {"timeWarpEnabled", "bad"}, {"timeWarpRepeats", "65"}, {"timeWarpAmount", "nan"},
        {"timeWarpQuantize", "3"}, {"timeWarpCurve", "missing"}, {"timeWarpNoteLengths", "bad"} })
    {
        auto bad = xml; bad.setAttribute(field.first, field.second);
        check(PatternBankFile::invalidAssignmentAttribute(bad) == field.first, "Bank accepted malformed warp settings");
    }
    xml.setAttribute("timeWarpExpression", juce::String::repeatedString("T", 513));
    check(PatternBankFile::invalidAssignmentAttribute(xml) == "timeWarpExpression", "Bank accepted oversized expression");
    auto bad = xml;
    bad.setAttribute("timeWarpAmount", "nan"); bad.setAttribute("timeWarpRepeats", 999);
    bad.setAttribute("timeWarpQuantize", 3); bad.setAttribute("timeWarpCurve", "unknown");
    const auto restored = AssignmentState::read(bad);
    check(restored.timeWarp.amount == 100 && restored.timeWarp.repeats == 64 && restored.timeWarp.quantize == 0
          && restored.timeWarp.curve == "lin" && restored.timeWarp.expression.size() == 512, "Host restore did not sanitize warp settings");
    juce::XmlElement legacy("Assignment");
    check(AssignmentState::read(legacy).timeWarp == Settings{}, "Old assignments must load with warp disabled");
}
