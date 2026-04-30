#include "Pcs12.h"

std::map<std::string, Pcs12> Pcs12::ChordDict;
std::map<Pcs12, std::string> Pcs12::ForteNumbersDict;
std::map<Pcs12, int> Pcs12::ForteNumbersRotationDict;
std::map<std::string, Pcs12> Pcs12::ForteNumbersToPCS12Dict;
std::map<std::string, std::string> Pcs12::ForteNumbersCommonNames;
