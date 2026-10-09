#include "../Source/Domain/PatternVariations.h"
#include "../Source/Domain/AssignmentState.h"
#include "../Source/Engine/GateRunnerEngine.h"
#include <stdexcept>
#include <set>
#include <iostream>

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << std::endl;
        throw std::runtime_error(message);
    }
}
}

void testPatternVariations()
{
    using namespace PatternVariations;
    KeyAssignment source;
    source.sequence = { 1, 2, 4, 8, 3, 5, 7, 0 };
    source.linkId = "shared-original";
    source.channel = 7; source.subdivision = 3; source.gate = 1.25f;
    source.fixedLengthSteps = 0.75f; source.rotation = -3; source.reverse = true;
    source.transpose = 4; source.velocity = 72;
    const auto before = source;
    Options options;
    auto family = build(source, options);
    check(family.valid() && family.permutation && family.transientApplications == 0
          && family.order == 4 && family.distinctPatterns == 4, "Quadratic cycle/order incorrect");
    check(family.variations[0].assignment.sequence == source.sequence, "Application zero must preserve the source");
    check(family.variations[1].assignment.sequence == std::vector<SequenceValue>({ 1, 8, 4, 5, 3, 0, 7, 2 }),
          "Quadratic step order incorrect");
    check(family.variations[4].repeatsKey == 60 && family.variations[5].repeatsKey == 61,
          "Repeated patterns not identified");
    for (const auto& variation : family.variations)
    {
        auto expected = source;
        expected.sequence = variation.assignment.sequence;
        expected.linkId.clear();
        check(variation.assignment == expected, "Variation lost source settings or retained a shared link");
        juce::XmlElement xml("Assignment");
        AssignmentState::write(xml, variation.assignment);
        check(AssignmentState::read(xml) == variation.assignment, "Baked variation did not round-trip");
    }
    check(source == before, "Preview changed its source");
    options.applicationsBetweenKeys = 2;
    family = build(source, options);
    check(family.valid() && family.distinctPatterns == 2 && family.variations[2].repeatsKey == 60,
          "Stride must compose the permutation multiple times");
    options.firstApplication = 3; options.applicationsBetweenKeys = 1; options.inverse = true;
    family = build(source, options);
    check(family.valid() && family.variations[0].assignment.sequence == build(source, Options{}).variations[1].assignment.sequence,
          "Inverse permutation or starting application incorrect");
    options = {};
    options.polynomial = { 0, 0, 2, 0 };
    family = build(source, options);
    check(family.valid() && !family.permutation && family.transientApplications == 3
          && family.order == 1 && family.distinctPatterns == 4, "Colliding map dynamics incorrect");
    check(family.variations[1].assignment.sequence == std::vector<SequenceValue>({1, 4, 3, 7, 1, 4, 3, 7})
          && family.variations[2].assignment.sequence == std::vector<SequenceValue>({1, 3, 1, 3, 1, 3, 1, 3})
          && family.variations[3].assignment.sequence == std::vector<SequenceValue>(8, 1)
          && family.variations[4].repeatsKey == 63, "Colliding map did not gather/composite source steps");
    check(source == before && family.variations[1].assignment.channel == source.channel
          && family.variations[1].assignment.linkId.empty(), "Mapping changed its source or assignment settings");
    options.inverse = true;
    check(!build(source, options).valid(), "A colliding polynomial acquired an inverse");
    std::vector<int> unchanged { 99 };
    std::string error;
    check(!compile(options.polynomial, 8, true, unchanged, error) && unchanged == std::vector<int>({99}),
          "Failed inverse compilation partially changed its destination");
    check(!compile(options.polynomial, 4097, false, unchanged, error) && unchanged == std::vector<int>({99}),
          "An oversized mapping was accepted or partially compiled");
    // Compare forward evaluation, permutation classification, inverse availability,
    // and repeated gathering against direct evaluation at prime/composite lengths.
    for (int length = 1; length <= 24; ++length)
        for (int quadratic = -2; quadratic <= 3; ++quadratic)
            for (int linear = -2; linear <= 3; ++linear)
            {
                Polynomial p { 1, quadratic, linear, -3 };
                std::set<int> expected;
                for (int i = 0; i < length; ++i)
                    expected.insert(modulo(static_cast<int64_t>(i) * i * i + quadratic * i * i + linear * i - 3, length));
                std::vector<int> map, inverse;
                const bool valid = compile(p, length, false, map, error);
                const bool permutation = expected.size() == static_cast<size_t>(length);
                check(valid && (dynamicsOf(map).transientApplications == 0) == permutation,
                      "Mapping acceptance or permutation classification incorrect");
                check(compile(p, length, true, inverse, error) == permutation, "Inverse availability incorrect");
                if (permutation)
                {
                    for (int i = 0; i < length; ++i)
                        check(inverse[static_cast<size_t>(map[static_cast<size_t>(i)])] == i, "Inverse map incorrect");
                }
                KeyAssignment fixture;
                fixture.sequence.clear();
                for (int i = 0; i < length; ++i) fixture.sequence.emplace_back(i + 1);
                Options composed;
                composed.polynomial = p; composed.firstApplication = 2;
                composed.applicationsBetweenKeys = 3; composed.keyCount = 6;
                const auto variations = build(fixture, composed);
                check(variations.valid() && variations.permutation == permutation,
                      ("General polynomial build failed at length " + std::to_string(length)
                       + ", quadratic " + std::to_string(quadratic) + ", linear " + std::to_string(linear)
                       + ": " + variations.error).c_str());
                for (const auto& variation : variations.variations)
                    for (int i = 0; i < length; ++i)
                    {
                        int from = i;
                        for (int application = 0; application < variation.application; ++application)
                            from = modulo(static_cast<int64_t>(from) * from * from + quadratic * from * from + linear * from - 3, length);
                        check(variation.assignment.sequence[static_cast<size_t>(i)] == fixture.sequence[static_cast<size_t>(from)],
                              "Repeated polynomial gathering disagrees with direct composition");
                    }
            }
    source.sequence = {1, 2, 3, 4, 5, 6};
    options = {}; options.polynomial = {0, 1, 0, 1};
    family = build(source, options);
    check(family.valid() && !family.permutation && family.transientApplications == 2 && family.order == 2
          && family.variations[2].assignment.sequence == std::vector<SequenceValue>({3, 6, 3, 6, 3, 6})
          && family.variations[3].assignment.sequence == std::vector<SequenceValue>({6, 3, 6, 3, 6, 3})
          && family.variations[4].repeatsKey == 62, "Transient leading to a nontrivial cycle incorrect");
    options.firstApplication = 65536; options.applicationsBetweenKeys = 4096;
    check(build(source, options).variations[7].assignment.sequence == family.variations[2].assignment.sequence,
          "Large colliding-map application/stride incorrect");
    options = {}; options.polynomial = {0, 0, 0, -1};
    family = build(source, options);
    check(family.valid() && family.transientApplications == 1 && family.order == 1
          && family.variations[1].assignment.sequence == std::vector<SequenceValue>(6, 6), "Constant map incorrect");
    source.sequence = { 1, 0, 2, 0, 3, 4 };
    options = {}; options.firstApplication = 1; options.keepRests = true;
    family = build(source, options);
    check(family.valid() && family.variations[0].assignment.sequence == std::vector<SequenceValue>({1, 0, 4, 0, 3, 2}),
          "Rest preservation permuted the wrong domain");
    options.polynomial = {0, 0, 2, 0};
    family = build(source, options);
    check(family.valid() && !family.permutation && family.transientApplications == 2
          && family.variations[0].assignment.sequence == std::vector<SequenceValue>({1, 0, 3, 0, 1, 3})
          && family.variations[1].assignment.sequence == std::vector<SequenceValue>({1, 0, 1, 0, 1, 1}),
          "Colliding map moved zero steps or used the full-length modulus");
    source.sequence = { 0, 0, 0 };
    check(build(source, options).valid() && build(source, options).distinctPatterns == 1,
          "All-rest loops must retain length");
    source.sequence.clear(); check(!build(source, options).valid(), "Empty sources must be rejected");
    source.sequence = { 2, 6, 8 };
    KeyAssignment flipped, twice;
    check(flip(source, flipped, error) && flipped.sequence == std::vector<SequenceValue>({8, 12, 2}),
          "Vertical bit reflection incorrect");
    check(flip(flipped, twice, error) && twice == source, "Two flips did not restore the source");
    source.sequence = { -2, 6, 8, 0 };
    check(flip(source, flipped, error) && flipped.sequence == std::vector<SequenceValue>({-8, 12, 2, 0}),
          "Vertical flip lost direction/rests");
    source.sequence = { std::numeric_limits<int>::min() };
    check(flip(source, flipped, error) && flipped == source, "INT_MIN reflection overflowed");
    source.sequence = { -1, std::numeric_limits<int>::min() };
    check(flip(source, flipped, error) && flipped.sequence == std::vector<SequenceValue>({std::numeric_limits<int>::min(), -1}),
          "Negative endpoint reflection overflowed");
    source.sequence = { 1, std::numeric_limits<int>::min() };
    twice = flipped;
    check(!flip(source, twice, error) && twice == flipped, "Unrepresentable positive reflection partially applied");

    source.mode = KeyAssignment::Mode::rhythmic;
    source.rotation = 0; source.reverse = false;
    source.drumLaneCount = 3; source.drumVelocityBits = 2; source.sequence = { 33, 0, 2 };
    check(flip(source, flipped, error) && flipped.sequence == std::vector<SequenceValue>({18, 0, 32}),
          "Drum flip must move velocity words as lanes");
    const auto events = GateRunnerEngine::computeAllStepEvents(flipped);
    check(events[0].size() == 2 && events[0][0].velocityLevel == 85 && events[0][1].velocityLevel == 42,
          "Drum flip reversed velocity encoding");
    check(flip(flipped, twice, error) && twice == source, "Drum flip did not round-trip");
    source.drumLaneCount = 16; source.drumVelocityBits = 7;
    check(source.setSequenceFromString("40564819207303340847894502572032 3"), "Wide mask fixture invalid");
    check(flip(source, flipped, error) && flipped.sequence[0].bitsAt(0, 7) == 1
          && flipped.sequence[1].bitsAt(105, 7) == 3, "Wide drum lanes truncated or velocities changed");
    check(flip(flipped, twice, error) && twice == source, "Wide drum flip did not round-trip");
    options = {}; options.operation = Operation::verticalFlip;
    family = build(source, options);
    check(family.valid() && family.order == 2 && family.variations[2].assignment.sequence == source.sequence,
          "Repeated vertical reflection incorrect");
    options.firstKey = 127; options.keyCount = 2;
    check(!build(source, options).valid(), "Key range crossed MIDI 127");
    options.keyCount = 1; options.firstApplication = 65536; options.applicationsBetweenKeys = 4096;
    check(build(source, options).valid(), "Valid application/key boundary rejected");
    source.mode = KeyAssignment::Mode::melodic;
    check(!build(source, options).valid(), "Wide melodic flip silently discarded masks");

    std::vector<int> map;
    check(compile({std::numeric_limits<int>::min(), 0, std::numeric_limits<int>::max(), 0}, 8, false, map, error)
          && map[1] == 7, "Extreme coefficients overflowed modular evaluation");
    source = KeyAssignment{}; source.sequence.assign(4096, 1);
    options = {}; options.firstKey = 127; options.keyCount = 1;
    options.polynomial = {0, 0, 1, 1}; options.firstApplication = 65536;
    check(build(source, options).valid(), "Maximum pattern size or large power failed");
    for (int i = 0; i < 4096; ++i) source.sequence[static_cast<size_t>(i)] = i + 1;
    options.polynomial = {0, 0, 2, 0};
    family = build(source, options);
    check(family.valid() && family.transientApplications == 12 && family.order == 1
          && family.variations[0].assignment.sequence == std::vector<SequenceValue>(4096, 1),
          "Maximum-length colliding map did not settle at a large power");
    source.sequence.assign(4096, 1); source.sequence[0] = std::numeric_limits<int>::min();
    options = {}; options.polynomial = {0, 0, 0, 0};
    family = build(source, options);
    check(!family.valid() && family.variations.empty() && family.error.find("pattern text limit") != std::string::npos,
          "Mapping generated an unreadable oversized pattern or exposed a partial family");
}
