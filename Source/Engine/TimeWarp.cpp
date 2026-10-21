#include "../Domain/TimeWarp.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <sstream>
#include <locale>
#include <stdexcept>
#include <variant>

namespace TimeWarp
{
namespace
{
constexpr double PI = 3.14159265358979323846;
using Sequence = std::vector<double>;
using Value = std::variant<double, Sequence>;

double number(const Value& value)
{
    if (!std::holds_alternative<double>(value) || !std::isfinite(std::get<double>(value)))
        throw std::runtime_error("Expected a finite number.");
    return std::get<double>(value);
}

double integerPart(double x) { return std::trunc(x); }
double sawWform(double x) { return (x - integerPart(x)) * 2 - 1; }
double revsawWform(double x) { return 1 - (x - integerPart(x)) * 2; }
double triangleWform(double x) { return -2 * std::asin(std::sin(2 * PI * (x - 0.5))) / PI; }

double wform(double x, const Sequence& args)
{
    if (args.size() < 3) return 0;
    Sequence values { args[0] }, durations;
    for (size_t i = 1; i < args.size(); i += 2)
    {
        durations.push_back(args[i]);
        values.push_back(i + 1 < args.size() ? args[i + 1] : values.back());
    }
    double steps = 0;
    for (const auto duration : durations) steps += duration;
    durations.push_back(std::max(0.0, 1 - steps));
    values.push_back(values.front());
    const double offset = integerPart(x), xx = x - offset;
    double elapsed = 0;
    for (size_t i = 0; i < durations.size(); ++i)
    {
        const auto duration = durations[i];
        elapsed += duration;
        if (elapsed > xx || i + 1 == durations.size())
        {
            if (duration <= 0) return offset + values[i + 1];
            return offset + values[i] + (values[i + 1] - values[i]) * ((xx - elapsed + duration) / duration);
        }
    }
    return offset + values.back();
}

double stowform(double x, const Sequence& sequence)
{
    if (sequence.size() < 2) return x;
    const auto maximum = *std::max_element(sequence.begin(), sequence.end());
    if (!std::isfinite(maximum) || maximum == 0) return x;
    Sequence args;
    for (size_t i = 0; i < sequence.size(); ++i)
    {
        args.push_back(sequence[i] / maximum);
        if (i + 1 < sequence.size()) args.push_back(1.0 / (sequence.size() - 1));
    }
    return wform(x, args);
}

const std::vector<std::string> functions {
    "sin", "cos", "tan", "asin", "acos", "atan", "sqrt", "pow", "abs", "min", "max",
    "floor", "ceil", "round", "integer", "exp", "log", "sign", "seq", "wform", "stowform",
    "saw_wform", "revsaw_wform", "triangle_wform"
};

Value call(const std::string& name, const std::vector<Value>& args)
{
    auto arg = [&](size_t i) { return i < args.size() ? number(args[i]) : 0.0; };
    if (name == "sin") return std::sin(arg(0));
    if (name == "cos") return std::cos(arg(0));
    if (name == "tan") return std::tan(arg(0));
    if (name == "asin") return std::asin(arg(0));
    if (name == "acos") return std::acos(arg(0));
    if (name == "atan") return std::atan(arg(0));
    if (name == "sqrt") return std::sqrt(arg(0));
    if (name == "pow") return std::pow(arg(0), arg(1));
    if (name == "abs") return std::abs(arg(0));
    if (name == "floor") return std::floor(arg(0));
    if (name == "ceil") return std::ceil(arg(0));
    if (name == "round") return std::floor(arg(0) + 0.5);
    if (name == "integer") return integerPart(arg(0));
    if (name == "exp") return std::exp(arg(0));
    if (name == "log") return std::log(arg(0));
    if (name == "sign") { const auto x = arg(0); return x == 0 ? 0.0 : x < 0 ? -1.0 : 1.0; }
    if (name == "saw_wform") return sawWform(arg(0));
    if (name == "revsaw_wform") return revsawWform(arg(0));
    if (name == "triangle_wform") return triangleWform(arg(0));
    if (name == "stowform")
    {
        const auto x = arg(0);
        if (args.size() < 2) return x;
        if (!std::holds_alternative<Sequence>(args[1])) throw std::runtime_error("Expected seq(...).");
        return stowform(x, std::get<Sequence>(args[1]));
    }
    if (name == "wform")
    {
        Sequence points;
        for (size_t i = 1; i < args.size(); ++i) points.push_back(arg(i));
        return wform(arg(0), points);
    }
    Sequence values;
    for (const auto& value : args) values.push_back(number(value));
    if (name == "seq") return values;
    if (values.empty()) return name == "min" ? std::numeric_limits<double>::infinity()
                                             : -std::numeric_limits<double>::infinity();
    return name == "min" ? *std::min_element(values.begin(), values.end())
                         : *std::max_element(values.begin(), values.end());
}

struct Node
{
    char kind = 'n';
    double literal = 0;
    std::string name;
    std::vector<std::shared_ptr<Node>> children;
    int height = 1;
    Value evaluate(double t) const
    {
        if (kind == 'n') return literal;
        if (kind == 'v') return t;
        if (kind == 'f')
        {
            std::vector<Value> args;
            for (const auto& child : children) args.push_back(child->evaluate(t));
            return call(name, args);
        }
        const auto left = number(children[0]->evaluate(t));
        if (kind == 'u') return name == "-" ? -left : left;
        const auto right = number(children[1]->evaluate(t));
        switch (kind)
        {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return right == 0 ? 0.0 : left / right;
            case '%': return right == 0 ? 0.0 : std::fmod(left, right);
            default: return std::pow(left, right);
        }
    }
};

class Parser
{
public:
    explicit Parser(const std::string& input) : source(input) {}
    std::shared_ptr<Node> parse()
    {
        if (source.size() > 512) fail("Expression exceeds 512 characters.");
        whitespace();
        if (position < source.size() && (source[position] == 'Y' || source[position] == 'y'))
        {
            ++position; whitespace();
            if (!take('=')) fail("Expected Y = expression.");
        }
        auto node = expression(1, 0);
        whitespace();
        if (position != source.size()) fail("Unexpected token.");
        return node;
    }
private:
    const std::string& source;
    size_t position = 0;
    int nodes = 0;
    [[noreturn]] void fail(const std::string& message) const { throw std::runtime_error(message); }
    void whitespace() { while (position < source.size() && std::isspace(static_cast<unsigned char>(source[position]))) ++position; }
    bool take(char c) { whitespace(); if (position < source.size() && source[position] == c) { ++position; return true; } return false; }
    std::shared_ptr<Node> make(char kind, double value = 0, std::string name = {},
                               std::vector<std::shared_ptr<Node>> children = {})
    {
        if (++nodes > 256) fail("Expression exceeds 256 nodes.");
        int height = 1;
        for (const auto& child : children) height = std::max(height, child->height + 1);
        if (height > 32) fail("Expression exceeds depth 32.");
        return std::make_shared<Node>(Node { kind, value, std::move(name), std::move(children), height });
    }
    std::shared_ptr<Node> expression(int minimum, int depth)
    {
        if (depth > 32) fail("Expression exceeds depth 32.");
        auto left = primary(depth);
        for (;;)
        {
            whitespace();
            const char op = position < source.size() ? source[position] : '\0';
            const int precedence = op == '+' || op == '-' ? 1 : op == '*' || op == '/' || op == '%' ? 2 : op == '^' ? 3 : 0;
            if (!precedence || precedence < minimum) return left;
            ++position;
            auto right = expression(op == '^' ? precedence : precedence + 1, depth + 1);
            left = make(op, 0, {}, { left, right });
        }
    }
    std::shared_ptr<Node> primary(int depth)
    {
        if (depth > 32) fail("Expression exceeds depth 32.");
        whitespace();
        if (position >= source.size()) fail("Expected a number, T, or function.");
        const char c = source[position];
        if (c == '+' || c == '-') { ++position; return make('u', 0, std::string(1, c), { primary(depth + 1) }); }
        if (take('('))
        {
            auto node = expression(1, depth + 1);
            if (!take(')')) fail("Expected ')'.");
            return node;
        }
        if ((c >= '0' && c <= '9') || c == '.')
        {
            // Scan decimal/scientific notation explicitly; reject hex, NaN,
            // infinity, and locale-specific number formats.
            const auto start = position;
            bool digit = false, dot = false;
            while (position < source.size())
            {
                const auto ch = source[position];
                if (ch >= '0' && ch <= '9') { digit = true; ++position; }
                else if (ch == '.' && !dot) { dot = true; ++position; }
                else if ((ch == 'e' || ch == 'E') && digit)
                {
                    auto next = position + 1;
                    if (next < source.size() && (source[next] == '+' || source[next] == '-')) ++next;
                    if (next >= source.size() || source[next] < '0' || source[next] > '9') break;
                    position = next + 1;
                    while (position < source.size() && source[position] >= '0' && source[position] <= '9') ++position;
                }
                else break;
            }
            if (!digit) fail("Invalid number.");
            // Classic locale, independent of the user's decimal separator.
            std::istringstream input(source.substr(start, position - start));
            input.imbue(std::locale::classic());
            double value = 0; input >> value;
            if (input.fail() || !std::isfinite(value)) fail("Invalid number.");
            return make('n', value);
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            const auto start = position++;
            while (position < source.size() && (std::isalnum(static_cast<unsigned char>(source[position])) || source[position] == '_')) ++position;
            const auto name = source.substr(start, position - start);
            if (name.size() > 32) fail("Identifier exceeds 32 characters.");
            if (take('('))
            {
                if (std::find(functions.begin(), functions.end(), name) == functions.end()) fail("Unknown function: " + name);
                std::vector<std::shared_ptr<Node>> args;
                if (!take(')'))
                {
                    do { args.push_back(expression(1, depth + 1)); } while (take(','));
                    if (!take(')')) fail("Expected ')'.");
                }
                return make('f', 0, name, std::move(args));
            }
            if (name == "T") return make('v');
            if (name == "PI") return make('n', PI);
            if (name == "E") return make('n', std::exp(1.0));
            fail("Unknown identifier: " + name);
        }
        fail("Invalid token.");
    }
};

double circularIn(double t) { return 1 - std::sqrt(std::max(0.0, 1 - t * t)); }
double circularOut(double t) { return std::sqrt(std::max(0.0, 1 - (t - 1) * (t - 1))); }
double circularInOut(double t) { return t < 0.5 ? circularIn(t * 2) / 2 : 0.5 + circularOut(t * 2 - 1) / 2; }
double backIn(double t) { return t * t * (2.70158 * t - 1.70158); }
double backOut(double t) { const auto u = t - 1; return 1 + 2.70158 * u * u * u + 1.70158 * u * u; }
double bounceOut(double t)
{
    const double n = 7.5625, d = 2.75;
    if (t < 1 / d) return n * t * t;
    if (t < 2 / d) { const auto u = t - 1.5 / d; return n * u * u + 0.75; }
    if (t < 2.5 / d) { const auto u = t - 2.25 / d; return n * u * u + 0.9375; }
    const auto u = t - 2.625 / d; return n * u * u + 0.984375;
}
std::function<double(double)> halfSinePower(double e) { return [=](double t) { return std::pow(std::sin(PI * t), e); }; }
std::function<double(double)> reversePower(double e) { return [=](double t) { return std::pow(1 - t, e); }; }
std::function<double(double)> elasticPulse(double amp, double freq) { return [=](double t) { return t + std::sin(PI * t) * amp * std::sin(freq * 2 * PI * t); }; }
std::function<double(double)> fmModel(int model, double amp, double freq, double modfreq, double modindex)
{
    return [=](double t)
    {
        const auto phase = freq * (t + modindex * std::sin(modfreq * 2 * PI * t));
        const auto envelope = model == 2 ? std::pow(std::sin(PI * t), 2) : model == 3 ? 1 - std::sin(PI * t) : std::sin(PI * t);
        const auto wave = model == 4 ? triangleWform(phase) : model == 5 ? sawWform(phase) : std::sin(phase * 2 * PI);
        return t + envelope * amp * wave;
    };
}
} // namespace

const std::vector<Curve>& curves()
{
    static const std::vector<Curve> library {
        { "lin", "Basic", [](double t) { return t; } },
        { "invlin", "Basic", [](double t) { return 1.0 - t; } },
        { "smoothstep", "Basic", [](double t) { return (t * t) * (3.0 - (2.0 * t)); } },
        { "smootherstep", "Basic", [](double t) { return (t * t * t) * (t * (t * 6.0 - 15.0) + 10.0); } },
        { "halfsine", "Half-Sine Power", halfSinePower(1.0) },
        { "hsp1over3", "Half-Sine Power", halfSinePower(1.0 / 3.0) },
        { "hsp1over2", "Half-Sine Power", halfSinePower(1.0 / 2.0) },
        { "hsp1", "Half-Sine Power", halfSinePower(1.0) },
        { "hsp2", "Half-Sine Power", halfSinePower(2.0) },
        { "hsp3", "Half-Sine Power", halfSinePower(3.0) },
        { "bump", "Bell and Fold", [](double t) { return std::sin(PI * t); } },
        { "dip", "Bell and Fold", [](double t) { return 1.0 - std::sin(PI * t); } },
        { "bell2", "Bell and Fold", [](double t) { return std::pow(std::sin(PI * t), 2.0); } },
        { "bell4", "Bell and Fold", [](double t) { return std::pow(std::sin(PI * t), 4.0); } },
        { "mirror", "Bell and Fold", [](double t) { return std::abs((2.0 * t) - 1.0); } },
        { "tent", "Bell and Fold", [](double t) { return 1.0 - std::abs((2.0 * t) - 1.0); } },
        { "rev", "Reverse Power", reversePower(1.0) },
        { "rp1over3", "Reverse Power", reversePower(1.0 / 3.0) },
        { "rp1over2", "Reverse Power", reversePower(1.0 / 2.0) },
        { "rp1", "Reverse Power", reversePower(1.0) },
        { "rp2", "Reverse Power", reversePower(2.0) },
        { "rp3", "Reverse Power", reversePower(3.0) },
        { "rp4", "Reverse Power", reversePower(4.0) },
        { "rp5", "Reverse Power", reversePower(5.0) },
        { "pow1over2", "Power", [](double t) { return std::pow(t, 1.0 / 2.0); } },
        { "pow1over3", "Power", [](double t) { return std::pow(t, 1.0 / 3.0); } },
        { "pow2over3", "Power", [](double t) { return std::pow(t, 2.0 / 3.0); } },
        { "pow2", "Power", [](double t) { return std::pow(t, 2.0); } },
        { "pow3", "Power", [](double t) { return std::pow(t, 3.0); } },
        { "pow4", "Power", [](double t) { return std::pow(t, 4.0); } },
        { "pow5", "Power", [](double t) { return std::pow(t, 5.0); } },
        { "powm1over3", "Power", [](double t) { return 1.0 - std::pow(t, 1.0 / 3.0); } },
        { "sqrt", "Power", [](double t) { return std::sqrt(t); } },
        { "step2", "Basic", [](double t) { return integerPart(t * 2) / 2.0; } },
        { "step4", "Basic", [](double t) { return integerPart(t * 4) / 4.0; } },
        { "step8", "Basic", [](double t) { return integerPart(t * 8) / 8.0; } },
        { "step16", "Basic", [](double t) { return integerPart(t * 16) / 16.0; } },
        { "circin", "Circular", circularIn },
        { "circout", "Circular", circularOut },
        { "circinout", "Circular", circularInOut },
        { "backin", "Overshoot", backIn },
        { "backout", "Overshoot", backOut },
        { "bouncein", "Overshoot", [](double t) { return 1.0 - bounceOut(1.0 - t); } },
        { "bounceout", "Overshoot", bounceOut },
        { "elastic1", "Overshoot", elasticPulse(0.1875, 5) },
        { "elastic2", "Overshoot", elasticPulse(0.25, 8) },
        { "sin1", "Wobble", [](double t) { return t + std::sin(2 * PI * t) * 0.25; } },
        { "sin2", "Wobble", [](double t) { return t + std::sin(2 * PI * 2 * t) * 0.1875; } },
        { "sin4", "Wobble", [](double t) { return t + std::sin(2 * PI * 4 * t) * 0.125; } },
        { "sin8", "Wobble", [](double t) { return t + std::sin(2 * PI * 8 * t) * 0.125; } },
        { "sin16", "Wobble", [](double t) { return t + std::sin(2 * PI * 16 * t) * 0.0625; } },
        { "tri4", "Wobble", [](double t) { return t + triangleWform(4 * t) * 0.1875; } },
        { "tri8", "Wobble", [](double t) { return t + triangleWform(8 * t) * 0.125; } },
        { "saw4", "Wobble", [](double t) { return t + sawWform(4 * t) * 0.1875; } },
        { "saw8", "Wobble", [](double t) { return t + sawWform(8 * t) * 0.125; } },
        { "revsaw4", "Wobble", [](double t) { return t + revsawWform(4 * t) * 0.1875; } },
        { "revsaw8", "Wobble", [](double t) { return t + revsawWform(8 * t) * 0.125; } },
        { "ripple", "Wobble", [](double t) { return t + std::sin(PI * t) * std::sin(12 * PI * t) * 0.0625; } },
        { "archseq", "Sequence Shapes", [](double t) { return stowform(t, {0, 2, 4, 6, 4, 2, 0}); } },
        { "zigzag", "Sequence Shapes", [](double t) { return stowform(t, {0, 4, 1, 5, 2, 6, 3, 7}); } },
        { "pendulum", "Sequence Shapes", [](double t) { return stowform(t, {0, 2, 4, 6, 8, 6, 4, 2, 0}); } },
        { "f1", "Sequence Shapes", [](double t) { return (wform(t * 8, {0, 0.25, 1, 0.25, 0, 0.25, -2}) * (1 / 16.0)) + wform(t, {0, 1, 1}); } },
        { "f2", "Sequence Shapes", [](double t) { return stowform(t, {0, 5, 6, 7, 4, 1, 2, 5, 8}); } },
        { "f3", "Sequence Shapes", [](double t) { return stowform(t, {1, 0, 2}); } },
        { "f4", "Sequence Shapes", [](double t) { return stowform(t, {0, 4, 2, 1, 4}); } },
        { "f5", "Sequence Shapes", [](double t) { return wform(t, {0, 0.0625, 0.0625, 0, 0.3125, 0.125, 0.4375, 0, 0.1875, 0.0625, 0.25, 0, 0.5, 0.25, 0.75, 0, 0.25, 0.0625, 0.3125, 0, 0.8125, 0.125, 0.9375, 0, 0.4375, 0.0625, 0.5, 0, 0.75, 0.0625, 0.8125, 0, 0.0625, 0.125, 0.1875, 0, 0.9375, 0.0625, 1}); } },
        { "f6", "Sequence Shapes", [](double t) { return wform(t, {0, 0.25, 0.25, 0, 0.375, 0.25, 0.625, 0, 0.75, 0.125, 0.875, 0, 0.25, 0.125, 0.375, 0, 0.875, 0.125, 1, 0, 0.625, 0.125, 0.75}); } },
        { "climb", "Sequence Shapes", [](double t) { return stowform(t, {0, 2, 1, 3, 2, 4, 3, 5}); } },
        { "valley", "Sequence Shapes", [](double t) { return stowform(t, {4, 2, 0, 2, 4}); } },
        { "hesitate", "Sequence Shapes", [](double t) { return stowform(t, {0, 0, 2, 2, 1, 3, 3, 4}); } },
        { "sin+x", "Wobble", [](double t) { return t + std::sin(8 * PI * t) * 0.125; } },
        { "f7", "Wobble", [](double t) { return t + triangleWform(t / 2.0) * 0.25 * triangleWform(8 * t); } },
        { "f8", "Wobble", [](double t) { return t + triangleWform(t / 2.0) * 0.25 * sawWform(8 * t); } },
        { "f9", "Wobble", [](double t) { return t + triangleWform(t) * 0.25 * triangleWform(8 * t); } },
        { "fm1", "FM", fmModel(1, 0.25, 8, 3, 0.125) },
        { "fm2", "FM", fmModel(1, 0.25, 4, 7, 0.25) },
        { "fm3", "FM", fmModel(1, 0.25, 16, 4, 0.125) },
        { "fm4", "FM", fmModel(1, 0.25, 12, 7, 0.25) },
        { "fm_gentle1", "FM", fmModel(1, 0.0625, 4, 2, 0.0625) },
        { "fm_gentle2", "FM", fmModel(1, 0.0625, 8, 3, 0.0625) },
        { "fm_mid1", "FM", fmModel(1, 0.125, 6, 2, 0.125) },
        { "fm_mid2", "FM", fmModel(1, 0.125, 8, 3, 0.125) },
        { "fm_mid3", "FM", fmModel(1, 0.1875, 12, 5, 0.125) },
        { "fm_strong1", "FM", fmModel(1, 0.25, 8, 3, 0.125) },
        { "fm_strong2", "FM", fmModel(1, 0.25, 12, 7, 0.25) },
        { "fm_strong3", "FM", fmModel(1, 0.3125, 16, 5, 0.25) },
        { "fm_center1", "FM", fmModel(2, 0.125, 8, 3, 0.125) },
        { "fm_center2", "FM", fmModel(2, 0.25, 12, 5, 0.25) },
        { "fm_edge1", "FM", fmModel(3, 0.125, 8, 3, 0.125) },
        { "fm_edge2", "FM", fmModel(3, 0.25, 12, 7, 0.25) },
        { "fm_tri1", "FM", fmModel(4, 0.125, 8, 3, 0.125) },
        { "fm_tri2", "FM", fmModel(4, 0.25, 12, 5, 0.25) },
        { "fm_saw1", "FM", fmModel(5, 0.125, 8, 3, 0.125) },
        { "fm_saw2", "FM", fmModel(5, 0.25, 16, 5, 0.25) },
    };
    return library;
}

bool isKnownCurve(const std::string& name)
{
    return name == "custom" || std::any_of(curves().begin(), curves().end(), [&](const Curve& curve) { return name == curve.name; });
}
bool isQuantize(int value) { return value == 0 || value == 1 || value == 2 || value == 4 || value == 8; }

Resolution compileExpression(const std::string& source)
{
    try
    {
        auto root = Parser(source).parse();
        return { [root](double t) { try { return number(root->evaluate(t)); } catch (...) { return t; } }, {} };
    }
    catch (const std::exception& error) { return { [](double t) { return t; }, error.what() }; }
}

Resolution resolve(const Settings& settings)
{
    if (settings.curve == "custom") return compileExpression(settings.expression);
    for (const auto& curve : curves()) if (settings.curve == curve.name) return { curve.function, {} };
    return { [](double t) { return t; }, "Unknown time warp curve." };
}

double normalized(double t, const Resolution& curve, double amount)
{
    const auto target = curve.function(t);
    return std::clamp(t + std::clamp(amount, 0.0, 1.0) * ((std::isfinite(target) ? target : t) - t), 0.0, 1.0);
}

Timing timing(double step, double duration, int length, const Settings& settings,
              const Resolution& curve, double minDuration)
{
    if (!settings.enabled || settings.amount <= 0 || length <= 0) return { step, duration };
    const auto repeats = std::clamp(settings.repeats, 1, 64);
    const auto chunk = static_cast<double>(length) / repeats;
    const auto chunkIndex = std::min(repeats - 1, static_cast<int>(std::floor(step / chunk)));
    const auto chunkStart = chunkIndex * chunk;
    const auto start = (step - chunkStart) / chunk;
    const auto end = std::min(1.0, start + duration / chunk);
    auto on = normalized(start, curve, settings.amount / 100);
    auto off = normalized(end, curve, settings.amount / 100);
    if (settings.quantize > 0 && isQuantize(settings.quantize))
    {
        const auto divisions = std::max(1.0, std::floor(chunk * settings.quantize + 0.5));
        if (divisions > 1)
        {
            on = std::clamp(std::floor(on * divisions + 0.5) / divisions, 0.0, 1.0);
            off = std::clamp(std::floor(off * divisions + 0.5) / divisions, 0.0, 1.0);
        }
    }
    return { chunkStart + on * chunk,
             settings.noteLengths ? std::max(minDuration, std::abs(off - on) * chunk) : duration };
}
} // namespace TimeWarp
