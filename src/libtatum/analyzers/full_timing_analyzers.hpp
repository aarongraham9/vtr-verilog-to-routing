#pragma once
#include <memory>
#include "TimingGraph.hpp"
#include "TimingConstraints.hpp"
#include "timing_analyzer_interfaces.hpp"

#include "SetupAnalysisVisitor.hpp"
#include "HoldAnalysisVisitor.hpp"
#include "SetupHoldAnalysisVisitor.hpp"

template<class GraphWalker, class DelayCalc>
class SetupFullTimingAnalyzer : public SetupTimingAnalyzer {
    public:
        SetupFullTimingAnalyzer(std::shared_ptr<TimingGraph> timing_graph, std::shared_ptr<TimingConstraints> timing_constraints, std::shared_ptr<DelayCalc> delay_calculator)
            : SetupTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints_)
            , delay_calculator_(delay_calculator)
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, setup_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, delay_calculator_, setup_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, setup_visitor_);            
        }

        virtual void reset_timing_impl() override {
            setup_visitor_.clear();
        }

        std::shared_ptr<TimingTags> get_setup_tags_impl(NodeId node_id) {
            return setup_visitor_.get_tags(node_id);
        }

    private:
        std::shared_ptr<TimingGraph> timing_graph_;
        std::shared_ptr<TimingConstraints> timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calculator_;
        SetupAnalysisVisitor setup_visitor_;
        GraphWalker<SetupAnalysisVisitor> graph_walker_;
};

template<class GraphWalker, class DelayCalc>
class HoldFullTimingAnalyzer : public HoldTimingAnalyzer {
    public:
        HoldFullTimingAnalyzer(std::shared_ptr<TimingGraph> timing_graph, std::shared_ptr<TimingConstraints> timing_constraints, std::shared_ptr<DelayCalc> delay_calculator)
            : HoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints_)
            , delay_calculator_(delay_calculator)
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, delay_calculator_, hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, hold_visitor_);            
        }

        virtual void reset_timing_impl() override {
            hold_visitor_.clear();
        }

        std::shared_ptr<TimingTags> get_hold_tags_impl(NodeId node_id) {
            return hold_visitor_.get_tags(node_id);
        }

    private:
        std::shared_ptr<TimingGraph> timing_graph_;
        std::shared_ptr<TimingConstraints> timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calculator_;
        HoldAnalysisVisitor hold_visitor_;
        GraphWalker<HoldAnalaysisVisitor> graph_walker_;
};

template<class GraphWalker, class DelayCalc>
class SetupHoldFullTimingAnalyzer : public SetupHoldTimingAnalyzer {
    public:
        SetupHoldFullTimingAnalyzer(std::shared_ptr<TimingGraph> timing_graph, std::shared_ptr<TimingConstraints> timing_constraints, std::shared_ptr<DelayCalc> delay_calculator)
            : SetupHoldTimingAnalyzer()
            , timing_graph_(timing_graph)
            , timing_constraints_(timing_constraints_)
            , delay_calculator_(delay_calculator)
            {}

    protected:
        virtual void update_timing_impl() override {
            graph_walker_.do_arrival_pre_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            
            graph_walker_.do_arrival_traversal(timing_graph_, timing_constraints_, setup_hold_visitor_);            

            graph_walker_.do_required_pre_traversal(timing_graph_, delay_calculator_, setup_hold_visitor_);            
            graph_walker_.do_required_traversal(timing_graph_, delay_calculator_, setup_hold_visitor_);            
        }

        virtual void reset_timing_impl() override {
            setup_hold_visitor_.clear();
        }

        std::shared_ptr<TimingTags> get_setup_tags(NodeId node_id) {
            return setup_hold_visitor_.get_setup_tags(node_id);
        }

        std::shared_ptr<TimingTags> get_hold_tags(NodeId node_id) {
            return setup_hold_visitor_.get_hold_tags(node_id);
        }

    private:
        std::shared_ptr<TimingGraph> timing_graph_;
        std::shared_ptr<TimingConstraints> timing_constraints_;
        std::shared_ptr<DelayCalc> delay_calculator_;
        SetupHoldAnalysisVisitor setup_hold_visitor_;
        GraphWalker<SetupHoldAnalysisVisistor> graph_walker_;
};
