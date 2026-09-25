#pragma once

#include <JuceHeader.h>
#include <string_view>

namespace SafeXml
{
// JUCE's XML reader recurses for child elements and resolves DTD entities.
// Validate a bounded, shallow document before handing it to that reader.
inline bool hasSafeStructure(const juce::String& text)
{
    const std::string_view xml(text.toRawUTF8(), text.getNumBytesAsUTF8());
    if (xml.empty() || xml.size() > 16 * 1024 * 1024) return false;
    size_t position = 0;
    int depth = 0, elements = 0;
    while ((position = xml.find('<', position)) != std::string_view::npos)
    {
        const auto tail = xml.substr(position);
        const auto skip = [&](std::string_view ending, size_t prefix) {
            const auto end = xml.find(ending, position + prefix);
            if (end == std::string_view::npos) return false;
            position = end + ending.size();
            return true;
        };
        if (tail.substr(0, 4) == "<!--") { if (!skip("-->", 4)) return false; continue; }
        if (tail.substr(0, 9) == "<![CDATA[") { if (!skip("]]>", 9)) return false; continue; }
        if (tail.substr(0, 2) == "<?") { if (!skip("?>", 2)) return false; continue; }
        if (tail.size() < 3 || tail[1] == '!') return false;
        const bool closing = tail[1] == '/';
        char quote = 0;
        int attributes = 0;
        size_t end = position + 1;
        for (; end < xml.size(); ++end)
        {
            const char c = xml[end];
            if (c == '<') return false;
            if (quote != 0) { if (c == quote) quote = 0; continue; }
            if (c == '\'' || c == '"') quote = c;
            else if (c == '=' && ++attributes > 64) return false;
            else if (c == '>') break;
        }
        if (end == xml.size()) return false;
        if (closing) { if (--depth < 0) return false; }
        else
        {
            if (++elements > 4096 || depth + 1 > 8) return false;
            if (xml[end - 1] != '/') ++depth;
        }
        position = end + 1;
    }
    return depth == 0 && elements > 0;
}

inline std::unique_ptr<juce::XmlElement> parse(const juce::String& text)
{
    return hasSafeStructure(text) ? juce::XmlDocument::parse(text) : nullptr;
}

inline std::unique_ptr<juce::XmlElement> readFile(const juce::File& file, int64_t limit)
{
    auto input = file.createInputStream();
    if (input == nullptr || input->getTotalLength() <= 0 || input->getTotalLength() > limit)
        return {};
    juce::MemoryBlock data;
    input->readIntoMemoryBlock(data, static_cast<juce::ssize_t>(limit + 1));
    if (data.getSize() == 0 || data.getSize() > static_cast<size_t>(limit)) return {};
    return parse(juce::String::createStringFromData(data.getData(), static_cast<int>(data.getSize())));
}
}
