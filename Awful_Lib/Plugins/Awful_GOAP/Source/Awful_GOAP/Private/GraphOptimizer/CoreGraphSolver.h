// Copyright Strati D. Zerbinis 2026. All Rights Reserved.
#pragma once
#include <chrono>
#include <functional>
#include <map>
#include <mutex>

#include "Utilities/PrioMap.h"

#include "BasicTypes.h"
#include "HeuristicBase.h"
#include "SequenceUnit.h"

// This class will find an optimized sequence, path, or ordering within a given graph. 
// 
// This represents to core functionality on top of which more specialized solvers
// can build on top of to provide pathfinding (A* solvers), constraint satisfaction, or planning (GOAP solver) 


// these specializations need to be in root namespace
template<>
struct std::hash<const Awful::GO::SequenceUnit<>*>
{
	std::size_t operator()(const Awful::GO::SequenceUnit<>* const& p) const noexcept
	{
		return  p->GetTargetNodeGUID();
	}
};

template<>
struct std::equal_to<const Awful::GO::SequenceUnit<>*>
{
	bool operator()(const Awful::GO::SequenceUnit<>* const& A, const Awful::GO::SequenceUnit<>* const& B) const
	{
		return A->GetTargetNodeGUID() == B->GetTargetNodeGUID();
	}
};


namespace Awful
{
	namespace GO
	{

		class NULL_StateExtension
		{
		};

		using DefaultStateExtension = NULL_StateExtension;

		class NULL_CRTPDerived
		{
		public:
			using StateExtension = NULL_StateExtension;
		};






		template<class GraphWorld, class HeuristicClass = NullHeuristicBase<typename GraphWorld::Node>, class Derived = NULL_CRTPDerived, class SequenceMetaData = DefaultSequenceBase, class StateExtension = DefaultStateExtension>
		class GraphCoreSolver
		{
		protected:
			using GraphSequenceUnit = SequenceUnit<SequenceMetaData>;

		private:
			// TODO look into improved containers/data layouts especially for rapid editing and localized invalidation
			using PathStack = std::map<PathGUID, int>;

			using SequenceStore = typename SequenceUnit<SequenceMetaData>::SequenceStore;

			using CRTP_Derived = Derived;

		public:
			using Node = typename GraphWorld::Node;
			using Edge = typename GraphWorld::Node::Edge;
			using ReferencePointType = typename GraphWorld::Node::Edge::ReferencePointType;

			// the iterator is here rather than in the sequence unit since here we have the concrete node/edge types
			class SequenceIterator
			{
				using iterator = typename SequenceStore::const_reverse_iterator;
			public:
				bool operator==(const SequenceIterator& aRHS) const { return mSequenceIterator == aRHS.mSequenceIterator; }
				bool operator!=(const SequenceIterator& aRHS) const { return mSequenceIterator != aRHS.mSequenceIterator; }
				SequenceIterator& operator++() { ++mSequenceIterator; return *this; }
				const Node& operator*()const { return mWorld.GetNode((*mSequenceIterator).GetTargetNodeGUID()); }

				const ReferencePointType* GetLink() const
				{
					const Node& node = mWorld.GetNode((*mSequenceIterator).GetSourceNodeGUID());
					const Edge* inEdge = node.GetEdge((*mSequenceIterator).GetEdgeID());
					if (inEdge != nullptr)
					{
						return &(inEdge->GetConnectionReference());
					}
					return nullptr;
				}
			private:
				friend class GraphCoreSolver;
				SequenceIterator(const GraphWorld& aWorld, iterator anIter)
					: mWorld(aWorld)
					, mSequenceIterator(anIter)
				{
				}

				const GraphWorld& mWorld;
				iterator mSequenceIterator;
			};


			GraphCoreSolver(const GraphWorld& aWorld);

			// TODO: 
			//  Not sure this is the most valid approach, should probably just lean into futures.
			SequenceIterator Begin(JobToken aToken);
			SequenceIterator End(JobToken aToken);

			const float Cost(JobToken aToken);

			// Should also specify maximum evaluation work either in terms of time, or steps, for throttling load
			void InitiateJob(JobToken aToken);
			JobToken InitiateJob(const ReferencePointType& aStart, const ReferencePointType& aDest, HeuristicClass& aHeuristic = sDefaultHeuristic, GraphOptimizerConfig* config = nullptr);


			// Currently Evaluate returns true if the evaluation has terminated and false otherwise. Might be better to either
			//	    register a callback
			//	    have a separate call IsDone(const JobToken&); that can be polled
			bool Evaluate(JobToken aToken);
			std::shared_future<bool> EvaluateAsync(JobToken& aToken);

			bool DestinationReached(JobToken aToken) const;
			void ReleaseToken(JobToken aToken);

		protected:
			// significant optimizations are possible, especially around access patterns and add/remove, or performing
			// operations on local subspaces of the graph

			// Optimization States are overly accessible, that's bad
			// Also probably hoist this out of the core solver and make it a private type
			class OptimizationState : public StateExtension
			{
				using BASE = StateExtension;
			public:
				OptimizationState()
					: BASE()
					, mLocalMapComparator(*this)
					, mHeuristic(reinterpret_cast<HeuristicClass*>(&sDefaultHeuristic)) // this is a super hack!
					, mActiveSequenceUnit(0)
					, mReferenceNumber(0)
				{
				}

				OptimizationState(HeuristicClass& aHeuristic)
					: BASE()
					, mLocalMapComparator(*this)
					, mCandidateMap(mLocalMapComparator)
					, mHeuristic(&aHeuristic)
					, mActiveSequenceUnit(0)
					, mReferenceNumber()
				{
					mCandidateMap.ResetComparator(mLocalMapComparator);
				}

				OptimizationState(const OptimizationState& aRHS)
					: BASE(aRHS)
					, mLocalMapComparator(*this)
					, mCandidateMap(aRHS.mCandidateMap)
					, mHeuristic(aRHS.mHeuristic)
					, mSequenceStore(aRHS.mSequenceStore)
					, mSequenceResult(aRHS.mSequenceResult)
					, mSourcePoint(aRHS.mSourcePoint)
					, mDestPoint(aRHS.mDestPoint)
					, mCurrent(aRHS.mCurrent)
					, mSource(aRHS.mSource)
					, mGoal(aRHS.mGoal)
					, mActiveSequenceUnit(aRHS.mActiveSequenceUnit)
					, mReferenceNumber(aRHS.mReferenceNumber)
				{
					mCandidateMap.ResetComparator(mLocalMapComparator);
				}


				OptimizationState(OptimizationState&& aRHS)
					: BASE(aRHS)
					, mLocalMapComparator(*this)
					, mCandidateMap(std::move(aRHS.mCandidateMap))
					, mHeuristic(aRHS.mHeuristic)
					, mSequenceStore(aRHS.mSequenceStore)
					, mSequenceResult(aRHS.mSequenceResult)
					, mSourcePoint(aRHS.mSourcePoint)
					, mDestPoint(aRHS.mDestPoint)
					, mCurrent(aRHS.mCurrent)
					, mSource(aRHS.mSource)
					, mGoal(aRHS.mGoal)
					, mActiveSequenceUnit(aRHS.mActiveSequenceUnit)
					, mReferenceNumber(aRHS.mReferenceNumber)
				{
					mCandidateMap.ResetComparator(mLocalMapComparator);
				}

				bool IsValid() const { return (mSource != sINVALID_Node) && (mGoal != sINVALID_Node); }

				OptimizationState& operator= (OptimizationState&& aRHS)
				{
					// Implementing this as forwarding to the constructor.
					// Ensure that the comparator function gets correctly
					// managed capturing the right this pointer

					// This feels clumsy but it will do
					this->~OptimizationState();
					new (this) OptimizationState(std::forward<OptimizationState>(aRHS));
					return *this;
				}

				void SetEndPoints(const GraphWorld& aWorld, const ReferencePointType& aStart, const ReferencePointType& aDest);
				void Initialize(const GraphWorld& aWorld);

				bool AtDestination() const { return mCurrent == mGoal; }
				void SetProcessed(const GraphSequenceUnit& aCandidate);
				void SetInitial(const GraphSequenceUnit& aCandidate);

				void AddCandidate(const GraphSequenceUnit& aCandidate);

				const GraphSequenceUnit& GetActiveUnit() const{ return mSequenceStore[mActiveSequenceUnit]; }
				GraphSequenceUnit& RegisterUnit(const PathGUID& aFromNodeGUID, const PathGUID& aToNodeGUID, int aEdgeID, float aHeuristicScore, float aTraversalCost);
				void RollBackUnit(GraphSequenceUnit& aUnit)
				{
					BASE::RollBackUnit(aUnit);
					mSequenceStore.pop_back();
				}

				const float GetCost() const { return mSequenceCost; }

				PathGUID GetCurrent() const { return mCurrent; }
				PathGUID GetSource() const { return mSource; }
				PathGUID GetGoal() const { return mGoal; }

				const GraphSequenceUnit* PopCandidate() { return GetSequenceUnit(mCandidateMap.Pop()); }

				bool CandidatesAvailable() { return !mCandidateMap.IsEmpty(); }

				bool NoMoreCandidates() const { return mCandidateMap.IsEmpty(); }
				
				bool IsDestination(PathGUID aCandidate) const { return aCandidate == mGoal; }
				bool IsDestination(const GraphSequenceUnit& aCandidate) const { return IsDestination(aCandidate.GetTargetNodeGUID()); }

				HeuristicClass* GetHeuristic() { return mHeuristic; }

				void ForcePrioSort() { mCandidateMap.PrioSort(); }

			private:
				friend class GraphCoreSolver;

				void Reset();
				const GraphSequenceUnit* GetSequenceUnit(int aPathUnitID) const;

				// the management of the heuristic is pretty clumsy, I should reevaluate this later
				struct HeuristicComparator
				{
					HeuristicComparator(OptimizationState& aOwner) :Owner(aOwner)
					{}
					OptimizationState& Owner;
					bool operator()(const int aLHS, const int aRHS)
					{
						const GraphSequenceUnit* left = Owner.GetSequenceUnit(aLHS);
						const GraphSequenceUnit* right = Owner.GetSequenceUnit(aRHS);
						return left->GetTotalCost() > right->GetTotalCost();
					}
				} mLocalMapComparator;

				using PrioMapType = PrioMap <PathGUID, int, HeuristicComparator>;
				PrioMapType mCandidateMap;

				HeuristicClass* mHeuristic = nullptr;

				ReferencePoint<ReferencePointType> mSourcePoint;
				ReferencePoint<ReferencePointType> mDestPoint;

				SequenceStore mSequenceStore;
				SequenceStore mSequenceResult;
				float mSequenceCost = 0.0f;

				PathGUID mCurrent = -1;
				PathGUID mSource = -1;
				PathGUID mGoal = -1;

				int mActiveSequenceUnit = 0;
				int mReferenceNumber = -1;

				GraphOptimizerConfig mConfig;

				std::chrono::high_resolution_clock::time_point mStartTime;
			};

			const GraphWorld& GetWorld() const { return mWorld; }
			OptimizationState& FindState(JobToken aToken);

			bool CheckForTimeout(OptimizationState& aState);
			JobToken GenerateToken() { return JobToken(++mTokenCounter); }
			void EnterCriticalSection() { mMutex.lock(); }
			void ExitCriticalSection() { mMutex.unlock(); }
		private:

			CRTP_Derived& GetDerived() { return *static_cast<CRTP_Derived*>(this); }
			const CRTP_Derived& GetDerived()const { return *static_cast<const CRTP_Derived*>(this); }

			static HeuristicClass sDefaultHeuristic;

			void MarshalPath(OptimizationState& aState);
				std::unordered_map<int, std::shared_ptr<OptimizationState>> mStateMap;

			using OptimizationStateList = std::deque<std::shared_ptr<OptimizationState>>;
			const GraphWorld& mWorld;
			OptimizationStateList mOptimizationStates;

			static OptimizationState sDefaultState;

			void InitOptStateInternal(OptimizationState& aState, const ReferencePointType& aStart, const ReferencePointType& aDest, GraphOptimizerConfig* optConfig = nullptr);
			void InitOptStateInternal(OptimizationState& aState);

			unsigned int mTokenCounter;
			std::mutex mMutex;
		};

		// ................................................................................
		//  static definitions
		// ................................................................................
		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename HeuristicClass GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::sDefaultHeuristic;

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::sDefaultState;

		// ................................................................................
		//  Inlined methods
		// ................................................................................

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::GraphCoreSolver(const GraphWorld& aWorld)
			: mWorld(aWorld)
			, mTokenCounter(0)
		{
		}

		// TODO, 
		//	In general, handle not found case!

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::SequenceIterator GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::Begin(JobToken aToken)
		{
			OptimizationState& state = FindState(aToken.mReferenceNumber);
			return SequenceIterator(mWorld, state.mSequenceResult.rbegin());
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::SequenceIterator GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::End(JobToken aToken)
		{
			OptimizationState& state = FindState(aToken.mReferenceNumber);
			return SequenceIterator(mWorld, state.mSequenceResult.rend());
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		const float GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::Cost(JobToken aToken)
		{
			// TODO, handle not found case!
			OptimizationState& state = FindState(aToken.mReferenceNumber);
			return state.GetCost();
		}


		// really it's reinitiate job
		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::InitiateJob(
			JobToken aToken)
		{
			OptimizationState& state = FindState(aToken);
			InitOptStateInternal(state);
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		JobToken GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::InitiateJob(
			const typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::ReferencePointType& aStart,
			const typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::ReferencePointType& aDest,
			HeuristicClass& aHeuristic, typename GraphOptimizerConfig* config/* = nullptr*/)
		{
			// critical section
			EnterCriticalSection();
			// TODO
			//  Pooling the states would improve speed and reduce resizing of temporary vectors
			// also this is a clumsy pattern

			// Create shared_ptr for thread-safe state management
			auto newState = std::make_shared<OptimizationState>(aHeuristic);
			mOptimizationStates.push_back(newState);

			// TODO
			//  Job tokens are only meaningful within the issuing solver. If there are multiple solvers we can have false tokens
			//  tokens should get a solver ID to disambiguate
			JobToken token = GenerateToken();
			newState->mReferenceNumber = token.mReferenceNumber;

			// Insert into state map for O(1) lookup
			mStateMap[token.mReferenceNumber] = newState;

			// end critical section
			// Release mutex before calling virtual/CRTP methods to avoid potential deadlocks
			ExitCriticalSection();

			InitOptStateInternal(*newState, aStart, aDest, config);
			// Re-acquire lock before returning token (defensive, though token is now safe)
			std::lock_guard<std::mutex> returnGuard(mMutex);

			return token;
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		bool GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::Evaluate(JobToken aToken)
		{
			OptimizationState& state = FindState(aToken);

			if (!state.IsValid())
			{
				return false;
			}

			// basic evaluation loop for a stack evaluator

			GetDerived().PushInitial(state);

			while (state.CandidatesAvailable())
			{
				if (!GetDerived().PopLowest(state))
				{
					return false;
				}

				if (state.AtDestination())
				{
					// well we're done then
					MarshalPath(state);
					return true;
				}

				GetDerived().AddCandidateNeighbours(state);

				if (CheckForTimeout(state))
				{
					// not necessarily a failure. We may need to pick this up where we left off though
					std::cout << "Timeout while evaluating a solution" << std::endl;
					return false;
				}
			}

			// TODO 
			//	 support store and resume for attenuating solves over multiple updates
			return false;
		}

		// just a wrapper for the evaluation as an async future
		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		std::shared_future<bool> GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::EvaluateAsync(JobToken& aToken)
		{
			// TODO
			//  look at the capture. 'this' is a terrible capture scope
			aToken.mFuture = std::async(std::launch::async, [this](JobToken token) {

				return Evaluate(token);

				}, aToken).share();

			return aToken.mFuture;
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		bool GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::DestinationReached(JobToken aToken) const
		{
			const OptimizationState& state = FindState(aToken);
			return state.AtDestination();
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::ReleaseToken(JobToken aToken)
		{
			std::lock_guard<std::mutex> guard(mMutex);

			// Remove state from map (shared_ptr reference will be released automatically)
			// This triggers the destructor which cleans up all nested resources:
			// - mCandidateMap (PrioMap)
			// - mSequenceStore, mSequenceResult (SequenceStore)
			// - mConfig, mHeuristic, mLocalMapComparator
			// - Time points, reference points, etc.
			auto mapIt = mStateMap.find(aToken.mReferenceNumber);
			if (mapIt != mStateMap.end()) {
				// Erasing from map decrements shared_ptr reference count
				// When count reaches zero, OptimizationState destructor is called
				mStateMap.erase(mapIt);
			}

			// Remove state from list
			auto listIt = std::find_if(mOptimizationStates.begin(), mOptimizationStates.end(),
				[&](const std::shared_ptr<OptimizationState>& state) {
					return state->mReferenceNumber == aToken.mReferenceNumber;
				});
			if (listIt != mOptimizationStates.end()) {
				// Erasing from list decrements shared_ptr reference count
				// This is safe even if already removed from map
				mOptimizationStates.erase(listIt);
			}
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState& GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::FindState(JobToken aToken)
		{
			std::lock_guard<std::mutex> guard(mMutex);

			// Use unordered_map for O(1) lookup instead of linear search
			auto it = mStateMap.find(aToken.mReferenceNumber);
			if (it != mStateMap.end()) {
				return *it->second;  // Dereference shared_ptr and return reference
			}

			// Fallback to default state if not found
			return sDefaultState;
		}



		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		bool GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::CheckForTimeout(OptimizationState& aState)
		{
			if (aState.mConfig.GetMaxDuration() < 0.0f)
			{
				return false;
			}

			auto currentTime = std::chrono::high_resolution_clock::now();
			const float duration = (float)std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - aState.mStartTime).count();

			return (duration >= (aState.mConfig.GetMaxDuration() * 1000.0f));
		}




		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::MarshalPath(typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState& aState)
		{
			GraphSequenceUnit const* current = &aState.GetActiveUnit();

			float sequenceCost = 0.0f;

			// build up sequence from nodes and edges tracing from goal to origin

			aState.mSequenceResult.clear();
			while (current != nullptr)
			{
				sequenceCost += current->GetTotalCost();

				aState.mSequenceResult.push_back(*current);
				current = aState.GetSequenceUnit(current->GetPrev());
			}
			aState.mSequenceCost = sequenceCost;
			aState.mSequenceStore.clear();
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::InitOptStateInternal(typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState& aState, const ReferencePointType& aStart, const ReferencePointType& aDest, GraphOptimizerConfig* config/* = nullptr*/)
		{
			InitOptStateInternal(aState);

			aState.SetEndPoints(mWorld, aStart, aDest);

			if (config != nullptr)
			{
				aState.mConfig = *config;
			}
		}

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::InitOptStateInternal(typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState& aState)
		{
			if (mWorld.IsEmpty())
			{
				return;
			}
			aState.Initialize(mWorld);
			aState.mStartTime = std::chrono::high_resolution_clock::now();
		}

		//*****************************************************************************
		// ************* Optimization State Functions
		//*****************************************************************************



		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::SetProcessed(const GraphSequenceUnit& aCandidate)
		{
			mCurrent = aCandidate.GetTargetNodeGUID();
			BASE::SetProcessed(aCandidate);
			mActiveSequenceUnit = aCandidate.GetID();
		}

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::SetInitial(const GraphSequenceUnit& aCandidate)
		{
			mCurrent = aCandidate.GetTargetNodeGUID();
			mActiveSequenceUnit = aCandidate.GetID();
		}

		// Simple and inefficient, improve the mappings since this will be high traffic
		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::AddCandidate(const GraphSequenceUnit& aCandidate)
		{
			const int candidateId = aCandidate.GetID();

			const PathGUID nodeId = aCandidate.GetTargetNodeGUID();

			if (mCandidateMap.Exists(nodeId))
			{
				auto& seqUnitIndex = mCandidateMap.PeekAt(nodeId);
				const GraphSequenceUnit* currentUnit = GetSequenceUnit(seqUnitIndex);

				// we've got a cheaper option 
				//if (currentUnit->GetHeuristicCost() > aCandidate.GetHeuristicCost())
				if (currentUnit->GetTotalCost() > aCandidate.GetTotalCost())
				{
					mCandidateMap.Edit(nodeId, candidateId);
				}
				return;
			}
			else
			{
				mCandidateMap.Insert(nodeId, candidateId);
			}
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::GraphSequenceUnit& GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::RegisterUnit(const PathGUID& aFromNodeGUID, const PathGUID& aToNodeGUID, int aEdgeID, float aHeuristicScore, float aTraversalCost)
		{
			int newID = (int)mSequenceStore.size();
			mSequenceStore.emplace_back(aFromNodeGUID, aToNodeGUID, aEdgeID, aHeuristicScore, aTraversalCost);
			GraphSequenceUnit& resultUnit = mSequenceStore.back();
			resultUnit.SetID(newID);
			return resultUnit;
		}


		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::Initialize(const GraphWorld& aWorld)
		{
			Reset();
		}

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::Reset()
		{
			BASE::Reset();
			mSequenceStore.clear();
			mCandidateMap.Clear();
			mSequenceResult.clear();
			mSequenceCost = 0.0f;
			mCurrent = mSource;
		}

		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		const typename GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::GraphSequenceUnit* GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::GetSequenceUnit(int aSequenceUnitID) const
		{
			if (aSequenceUnitID == GraphSequenceUnit::InvalidID)
			{
				return nullptr;
			}

			return &mSequenceStore[aSequenceUnitID];
		}



		template<class GraphWorld, class HeuristicClass, class Derived, class SequenceMetaData, class StateExtension>
		void GraphCoreSolver<GraphWorld, HeuristicClass, Derived, SequenceMetaData, StateExtension>::OptimizationState::SetEndPoints(const GraphWorld& aWorld, const ReferencePointType& aStart, const ReferencePointType& aDest)
		{
			mSourcePoint = aStart;
			mDestPoint = aDest;

			mSource = aWorld.FindNearestNode(mSourcePoint).GetInternalGUID();
			mGoal = aWorld.FindNearestNode(mDestPoint).GetInternalGUID();
		}

	}
}