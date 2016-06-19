#pragma once

#include "decode/source_phrase.hh"
#include "decode/feature_init.hh"
#include "decode/score_collector.hh"

#include <cstddef>
#include <string>

namespace decode {

// Layouts:
struct Hypothesis;
struct VocabWord;
 
struct PhrasePair {
  const SourcePhrase source_phrase;
  const TargetPhrase &target_phrase; // TODO make non-const once replaced with chart wrapper for row
};

class Feature {
  public:
    // recommended constructor: Feature(const std::string &config);
    Feature(const StringPiece feature_name) : name(feature_name) {}

    virtual ~Feature(){};
    
    StringPiece name;

    virtual void Init(FeatureInit &feature_init) = 0;

    virtual void NewWord(StringPiece string_rep, VocabWord *word) const = 0;

    virtual void ScorePhrase(PhrasePair phrase_pair, ScoreCollector &collector) const = 0;

    virtual void ScoreHypothesisWithSourcePhrase(
        const Hypothesis &hypothesis, const SourcePhrase source_phrase, ScoreCollector &collector) const = 0;

    virtual void ScoreHypothesisWithPhrasePair(
        const Hypothesis &hypothesis, PhrasePair phrase_pair, ScoreCollector &collector) const = 0;

    virtual void ScoreFinalHypothesis(
        const Hypothesis &hypothesis, ScoreCollector &collector) const = 0;

    virtual std::size_t DenseFeatureCount() const = 0;

    virtual std::string FeatureDescription(std::size_t index) const = 0;
};

} // namespace decode
