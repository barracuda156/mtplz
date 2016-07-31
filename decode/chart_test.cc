#include "decode/chart.hh"

#include "decode/system.hh"
#include "pt/access.hh"
#include "lm/model.hh"

#include <string>
#include <unordered_map>

#define BOOST_TEST_MODULE ChartTest
#include <boost/test/unit_test.hpp>

namespace decode {
namespace {

typedef std::pair<std::size_t,std::size_t> Range;

class FeatureMock : public Feature, public TargetPhraseInitializer {
  public:
    FeatureMock() : Feature("mock") {}
    FeatureMock(std::vector<StringPiece> &rep_buffer, std::vector<VocabWord*> &word_buffer)
      : Feature("mock"), rep_buffer_(&rep_buffer), word_buffer_(&word_buffer) {}
    FeatureMock(std::vector<PhrasePair> &phrase_pair_buffer)
      : Feature("mock"), phrase_pair_buffer_(&phrase_pair_buffer) {}

    void Init(FeatureInit &feature_init) override {}
    void NewWord(const StringPiece string_rep, VocabWord *word) const override {
      rep_buffer_->push_back(string_rep);
      word_buffer_->push_back(word);
    }
    void InitPassthroughPhrase(pt::Row *passthrough) const override {}
    void ScorePhrase(PhrasePair phrase_pair, ScoreCollector &collector) const override {
      phrase_pair_buffer_->push_back(phrase_pair);
    }
    void ScoreHypothesisWithSourcePhrase(
        const Hypothesis &hypothesis, const SourcePhrase source_phrase, ScoreCollector &collector) const override {}
    void ScoreHypothesisWithPhrasePair(
        const Hypothesis &hypothesis, PhrasePair phrase_pair, ScoreCollector &collector) const override {}
    void ScoreFinalHypothesis(
        const Hypothesis &hypothesis, ScoreCollector &collector) const override {}
    std::size_t DenseFeatureCount() const override { return 1; }
    std::string FeatureDescription(std::size_t index) const override { return ""; }
    void ScoreTargetPhrase(PhrasePair pair, lm::ngram::ChartState &state) const override {}

    std::vector<StringPiece> *rep_buffer_;
    std::vector<VocabWord*> *word_buffer_;
    std::vector<PhrasePair> *phrase_pair_buffer_;
};

BOOST_AUTO_TEST_CASE(InitTest) {
  pt::FieldConfig config;
  pt::Access access(config);
  lm::ngram::State lm_state;
  Objective objective(access, lm_state);
  FeatureMock feature_mock;
  objective.RegisterLanguageModel(feature_mock);
  BaseVocab base_vocab;
  base_vocab.map.push_back(nullptr);
  VocabMap vocab_map(objective, base_vocab);
  Chart chart(13, vocab_map, objective);
  BOOST_CHECK_EQUAL(13, chart.MaxSourcePhraseLength());
}

BOOST_AUTO_TEST_CASE(EosTest) {
  pt::FieldConfig config;
  pt::Access access(config);
  lm::ngram::State lm_state;
  Objective objective(access, lm_state);
  std::vector<PhrasePair> phrase_pair_buffer;
  FeatureMock feature_mock(phrase_pair_buffer);
  objective.AddFeature(feature_mock);
  objective.RegisterLanguageModel(feature_mock);
  BaseVocab base_vocab;
  base_vocab.map.push_back(nullptr);
  VocabMap vocab_map(objective, base_vocab);
  Chart chart(11, vocab_map, objective);

  TargetPhrases &eos = chart.EndOfSentence();
  BOOST_CHECK_EQUAL(1, eos.Root().Size());
  ID eos_word = Chart::EOS_WORD;
  const pt::Row *eos_row = objective.GetFeatureInit().pt_row_field(eos.Root().End().cvp);
  BOOST_CHECK_EQUAL(1, access.target(eos_row).size());
  BOOST_CHECK_EQUAL(eos_word, access.target(eos_row)[0]);
  BOOST_CHECK_EQUAL(1, phrase_pair_buffer.size());
}

BOOST_AUTO_TEST_CASE(ReadSentenceTest) {
  // init chart
  pt::FieldConfig config;
  pt::Access access(config);
  lm::ngram::State lm_state;
  Objective objective(access, lm_state);
  std::vector<StringPiece> rep_buffer;
  std::vector<VocabWord*> word_buffer;
  FeatureMock feature_mock(rep_buffer, word_buffer);
  objective.AddFeature(feature_mock);
  objective.RegisterLanguageModel(feature_mock);
  BaseVocab base_vocab;
  base_vocab.vocab.FindOrInsert("small");
  const std::size_t orig_vocabsize = base_vocab.vocab.Size();
  util::Layout &word_layout = objective.GetFeatureInit().word_layout;
  VocabWord *word_small = reinterpret_cast<VocabWord*>(word_layout.Allocate(base_vocab.pool));
  for (std::size_t i=0; i < orig_vocabsize-1; ++i) {
    base_vocab.map.push_back(nullptr);
  }
  base_vocab.map.push_back(word_small);
  VocabMap vocab_map(objective, base_vocab);
  Chart chart(5, vocab_map, objective);

  // test known and unknown
  std::string input = "a small test test";
  chart.ReadSentence(input);
  // vocab update
  BOOST_CHECK_EQUAL("a", vocab_map.String(orig_vocabsize));
  BOOST_CHECK_EQUAL("test", vocab_map.String(orig_vocabsize+1));
  // lengths
  BOOST_CHECK_EQUAL(4, chart.SentenceLength());
  BOOST_CHECK_EQUAL(2, word_buffer.size());
  BOOST_CHECK_EQUAL(2, rep_buffer.size());
  // sentence
  BOOST_CHECK_EQUAL(word_buffer[0], chart.Sentence()[0]);
  BOOST_CHECK_EQUAL(word_small, chart.Sentence()[1]);
  BOOST_CHECK_EQUAL(word_buffer[1], chart.Sentence()[2]);
  BOOST_CHECK_EQUAL(word_buffer[1], chart.Sentence()[3]);
  // new word calls
  BOOST_CHECK_EQUAL("a", rep_buffer[0]);
  BOOST_CHECK_EQUAL("test", rep_buffer[1]);
}

} // namespace
} // namespace decode
