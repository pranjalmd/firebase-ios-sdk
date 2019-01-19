/*
 * Copyright 2019 Google
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_WATCH_CHANGE_H_
#define FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_WATCH_CHANGE_H_

#if !defined(__OBJC__)
// TODO(varconst): the only dependencies are `FSTMaybeDocument` and `NSData`
// (the latter is used to represent the resume token).
#error "This header only supports Objective-C++"
#endif  // !defined(__OBJC__)

#import <Foundation/Foundation.h>

#include <utility>

#include "Firestore/core/src/firebase/firestore/model/document_key.h"
#include "Firestore/core/src/firebase/firestore/model/types.h"
#include "Firestore/core/src/firebase/firestore/remote/existence_filter.h"
#include "Firestore/core/src/firebase/firestore/util/status.h"

@class FSTMaybeDocument;

namespace firebase {
namespace firestore {
namespace remote {

/**
 * `WatchChange` is the internal representation of the watcher API protocol
 * buffers. This is an empty abstract class so that all the different kinds of
 * changes can have a common base class.
 */
class WatchChange {
 public:
  enum class Type {
    Document,
    ExistenceFilter,
    TargetChange,
  };

  explicit WatchChange(Type type) : type_{type} {
  }
  virtual ~WatchChange() {
  }

  Type type() const {
    return type_;
  }

  const Type type_;
};

/**
 * `DocumentWatchChange` represents a changed document and a list of target ids
 * to which this change applies.
 *
 * If document has been deleted, a `DeletedDocument` will be provided.
 */
class DocumentWatchChange : public WatchChange {
 public:
  DocumentWatchChange(std::vector<model::TargetId> updated_target_ids,
                      std::vector<model::TargetId> removed_target_ids,
                      model::DocumentKey document_key,
                      FSTMaybeDocument* new_document)
      : WatchChange{Type::Document},
        updated_target_ids_{std::move(updated_target_ids)},
        removed_target_ids_{std::move(removed_target_ids)},
        document_key_{std::move(document_key)},
        new_document_{new_document} {
  }

  /** The new document applies to all of these targets. */
  const std::vector<model::TargetId>& updated_target_ids() const {
    return updated_target_ids_;
  }

  /** The new document is removed from all of these targets. */
  const std::vector<model::TargetId>& removed_target_ids() const {
    return removed_target_ids_;
  }

  /**
   * The new document, or `DeletedDocument` if it was deleted. Is null if the
   * document went out of view without the server sending a new document.
   */
  FSTMaybeDocument* new_document() const {
    return new_document_;
  }

  /** The key of the document for this change. */
  const model::DocumentKey& document_key() const {
    return document_key_;
  }

 private:
  std::vector<model::TargetId> updated_target_ids_;
  std::vector<model::TargetId> removed_target_ids_;
  model::DocumentKey document_key_;
  FSTMaybeDocument* new_document_;
};

bool operator==(const DocumentWatchChange& lhs,
                       const DocumentWatchChange& rhs);

/**
 * An `ExistenceFilterWatchChange` applies to the targets and is required to
 * verify the current client state against expected state sent from the server.
 */
class ExistenceFilterWatchChange : public WatchChange {
 public:
  ExistenceFilterWatchChange(ExistenceFilter filter, model::TargetId target_id)
      : WatchChange{Type::ExistenceFilter},
        filter_{filter},
        target_id_{target_id} {
  }

  const ExistenceFilter& filter() const {
    return filter_;
  }
  model::TargetId target_id() const {
    return target_id_;
  }

 private:
  ExistenceFilter filter_;
  model::TargetId target_id_;
};

bool operator==(const ExistenceFilterWatchChange& lhs,
                       const ExistenceFilterWatchChange& rhs);

enum class WatchTargetChangeState { NoChange, Added, Removed, Current, Reset };

class WatchTargetChange : public WatchChange {
 public:
  WatchTargetChange(WatchTargetChangeState state,
                    std::vector<model::TargetId> target_ids)
      : WatchTargetChange{state, std::move(target_ids), [NSData data],
                          util::Status::OK()} {
  }

  WatchTargetChange(WatchTargetChangeState state,
                    std::vector<model::TargetId> target_ids,
                    NSData* resume_token)
      : WatchTargetChange{state, std::move(target_ids), resume_token,
                          util::Status::OK()} {
  }

  WatchTargetChange(WatchTargetChangeState state,
                    std::vector<model::TargetId> target_ids,
                    util::Status cause)
      : WatchTargetChange{state, std::move(target_ids), [NSData data], cause} {
  }

  WatchTargetChange(WatchTargetChangeState state,
                    std::vector<model::TargetId> target_ids,
                    NSData* resume_token,
                    util::Status cause)
      : WatchChange{Type::TargetChange},
        state_{state},
        target_ids_{std::move(target_ids)},
        resume_token_{resume_token},
        cause_{std::move(cause)} {
  }

  /** What kind of change occurred to the watch target. */
  WatchTargetChangeState state() const {
    return state_;
  }

  /** The target IDs that were added/removed/set. */
  const std::vector<model::TargetId>& target_ids() const {
    return target_ids_;
  }

  /**
   * An opaque, server-assigned token that allows watching a query to be
   * resumed after disconnecting without retransmitting all the data that
   * matches the query. The resume token essentially identifies a point in
   * time from which the server should resume sending results.
   */
  NSData* resume_token() const {
    return resume_token_;
  }

  /**
   * An RPC error indicating why the watch failed. Only valid if
   * WatchChangeState == Removed.
   */
  const util::Status& cause() const {
    return cause_;
  }

 private:
  WatchTargetChangeState state_;
  std::vector<model::TargetId> target_ids_;
  NSData* resume_token_;
  util::Status cause_;
};

bool operator==(const WatchTargetChange& lhs,
                       const WatchTargetChange& rhs);

}  // namespace remote
}  // namespace firestore
}  // namespace firebase

#endif  // FIRESTORE_CORE_SRC_FIREBASE_FIRESTORE_REMOTE_WATCH_CHANGE_H_
