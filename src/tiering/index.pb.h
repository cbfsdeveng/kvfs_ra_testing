// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: index.proto

#ifndef PROTOBUF_index_2eproto__INCLUDED
#define PROTOBUF_index_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace pqfs {
namespace proto {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_index_2eproto();
void protobuf_AssignDesc_index_2eproto();
void protobuf_ShutdownFile_index_2eproto();

class Index;
class InodeOffset;
class Segment;

// ===================================================================

class Index : public ::google::protobuf::Message {
 public:
  Index();
  virtual ~Index();

  Index(const Index& from);

  inline Index& operator=(const Index& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const Index& default_instance();

  void Swap(Index* other);

  // implements Message ----------------------------------------------

  Index* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Index& from);
  void MergeFrom(const Index& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int64 magic = 1;
  inline bool has_magic() const;
  inline void clear_magic();
  static const int kMagicFieldNumber = 1;
  inline ::google::protobuf::int64 magic() const;
  inline void set_magic(::google::protobuf::int64 value);

  // optional int64 version = 2;
  inline bool has_version() const;
  inline void clear_version();
  static const int kVersionFieldNumber = 2;
  inline ::google::protobuf::int64 version() const;
  inline void set_version(::google::protobuf::int64 value);

  // optional int64 fs_id = 3;
  inline bool has_fs_id() const;
  inline void clear_fs_id();
  static const int kFsIdFieldNumber = 3;
  inline ::google::protobuf::int64 fs_id() const;
  inline void set_fs_id(::google::protobuf::int64 value);

  // optional int64 first_xid = 4;
  inline bool has_first_xid() const;
  inline void clear_first_xid();
  static const int kFirstXidFieldNumber = 4;
  inline ::google::protobuf::int64 first_xid() const;
  inline void set_first_xid(::google::protobuf::int64 value);

  // optional int64 last_xid = 5;
  inline bool has_last_xid() const;
  inline void clear_last_xid();
  static const int kLastXidFieldNumber = 5;
  inline ::google::protobuf::int64 last_xid() const;
  inline void set_last_xid(::google::protobuf::int64 value);

  // repeated .pqfs.proto.Segment segment = 6;
  inline int segment_size() const;
  inline void clear_segment();
  static const int kSegmentFieldNumber = 6;
  inline const ::pqfs::proto::Segment& segment(int index) const;
  inline ::pqfs::proto::Segment* mutable_segment(int index);
  inline ::pqfs::proto::Segment* add_segment();
  inline const ::google::protobuf::RepeatedPtrField< ::pqfs::proto::Segment >&
      segment() const;
  inline ::google::protobuf::RepeatedPtrField< ::pqfs::proto::Segment >*
      mutable_segment();

  // @@protoc_insertion_point(class_scope:pqfs.proto.Index)
 private:
  inline void set_has_magic();
  inline void clear_has_magic();
  inline void set_has_version();
  inline void clear_has_version();
  inline void set_has_fs_id();
  inline void clear_has_fs_id();
  inline void set_has_first_xid();
  inline void clear_has_first_xid();
  inline void set_has_last_xid();
  inline void clear_has_last_xid();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int64 magic_;
  ::google::protobuf::int64 version_;
  ::google::protobuf::int64 fs_id_;
  ::google::protobuf::int64 first_xid_;
  ::google::protobuf::int64 last_xid_;
  ::google::protobuf::RepeatedPtrField< ::pqfs::proto::Segment > segment_;
  friend void  protobuf_AddDesc_index_2eproto();
  friend void protobuf_AssignDesc_index_2eproto();
  friend void protobuf_ShutdownFile_index_2eproto();

  void InitAsDefaultInstance();
  static Index* default_instance_;
};
// -------------------------------------------------------------------

class InodeOffset : public ::google::protobuf::Message {
 public:
  InodeOffset();
  virtual ~InodeOffset();

  InodeOffset(const InodeOffset& from);

  inline InodeOffset& operator=(const InodeOffset& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const InodeOffset& default_instance();

  void Swap(InodeOffset* other);

  // implements Message ----------------------------------------------

  InodeOffset* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const InodeOffset& from);
  void MergeFrom(const InodeOffset& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional int64 inode = 1;
  inline bool has_inode() const;
  inline void clear_inode();
  static const int kInodeFieldNumber = 1;
  inline ::google::protobuf::int64 inode() const;
  inline void set_inode(::google::protobuf::int64 value);

  // optional int64 offset = 2;
  inline bool has_offset() const;
  inline void clear_offset();
  static const int kOffsetFieldNumber = 2;
  inline ::google::protobuf::int64 offset() const;
  inline void set_offset(::google::protobuf::int64 value);

  // optional string pathname = 3;
  inline bool has_pathname() const;
  inline void clear_pathname();
  static const int kPathnameFieldNumber = 3;
  inline const ::std::string& pathname() const;
  inline void set_pathname(const ::std::string& value);
  inline void set_pathname(const char* value);
  inline void set_pathname(const char* value, size_t size);
  inline ::std::string* mutable_pathname();
  inline ::std::string* release_pathname();
  inline void set_allocated_pathname(::std::string* pathname);

  // @@protoc_insertion_point(class_scope:pqfs.proto.InodeOffset)
 private:
  inline void set_has_inode();
  inline void clear_has_inode();
  inline void set_has_offset();
  inline void clear_has_offset();
  inline void set_has_pathname();
  inline void clear_has_pathname();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::google::protobuf::int64 inode_;
  ::google::protobuf::int64 offset_;
  ::std::string* pathname_;
  friend void  protobuf_AddDesc_index_2eproto();
  friend void protobuf_AssignDesc_index_2eproto();
  friend void protobuf_ShutdownFile_index_2eproto();

  void InitAsDefaultInstance();
  static InodeOffset* default_instance_;
};
// -------------------------------------------------------------------

class Segment : public ::google::protobuf::Message {
 public:
  Segment();
  virtual ~Segment();

  Segment(const Segment& from);

  inline Segment& operator=(const Segment& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const Segment& default_instance();

  void Swap(Segment* other);

  // implements Message ----------------------------------------------

  Segment* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Segment& from);
  void MergeFrom(const Segment& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string name = 1;
  inline bool has_name() const;
  inline void clear_name();
  static const int kNameFieldNumber = 1;
  inline const ::std::string& name() const;
  inline void set_name(const ::std::string& value);
  inline void set_name(const char* value);
  inline void set_name(const char* value, size_t size);
  inline ::std::string* mutable_name();
  inline ::std::string* release_name();
  inline void set_allocated_name(::std::string* name);

  // optional int64 checksum = 2;
  inline bool has_checksum() const;
  inline void clear_checksum();
  static const int kChecksumFieldNumber = 2;
  inline ::google::protobuf::int64 checksum() const;
  inline void set_checksum(::google::protobuf::int64 value);

  // optional int64 first_xid = 3;
  inline bool has_first_xid() const;
  inline void clear_first_xid();
  static const int kFirstXidFieldNumber = 3;
  inline ::google::protobuf::int64 first_xid() const;
  inline void set_first_xid(::google::protobuf::int64 value);

  // optional int64 last_xid = 4;
  inline bool has_last_xid() const;
  inline void clear_last_xid();
  static const int kLastXidFieldNumber = 4;
  inline ::google::protobuf::int64 last_xid() const;
  inline void set_last_xid(::google::protobuf::int64 value);

  // repeated .pqfs.proto.InodeOffset inode_offset = 5;
  inline int inode_offset_size() const;
  inline void clear_inode_offset();
  static const int kInodeOffsetFieldNumber = 5;
  inline const ::pqfs::proto::InodeOffset& inode_offset(int index) const;
  inline ::pqfs::proto::InodeOffset* mutable_inode_offset(int index);
  inline ::pqfs::proto::InodeOffset* add_inode_offset();
  inline const ::google::protobuf::RepeatedPtrField< ::pqfs::proto::InodeOffset >&
      inode_offset() const;
  inline ::google::protobuf::RepeatedPtrField< ::pqfs::proto::InodeOffset >*
      mutable_inode_offset();

  // @@protoc_insertion_point(class_scope:pqfs.proto.Segment)
 private:
  inline void set_has_name();
  inline void clear_has_name();
  inline void set_has_checksum();
  inline void clear_has_checksum();
  inline void set_has_first_xid();
  inline void clear_has_first_xid();
  inline void set_has_last_xid();
  inline void clear_has_last_xid();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* name_;
  ::google::protobuf::int64 checksum_;
  ::google::protobuf::int64 first_xid_;
  ::google::protobuf::int64 last_xid_;
  ::google::protobuf::RepeatedPtrField< ::pqfs::proto::InodeOffset > inode_offset_;
  friend void  protobuf_AddDesc_index_2eproto();
  friend void protobuf_AssignDesc_index_2eproto();
  friend void protobuf_ShutdownFile_index_2eproto();

  void InitAsDefaultInstance();
  static Segment* default_instance_;
};
// ===================================================================


// ===================================================================

// Index

// optional int64 magic = 1;
inline bool Index::has_magic() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Index::set_has_magic() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Index::clear_has_magic() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Index::clear_magic() {
  magic_ = GOOGLE_LONGLONG(0);
  clear_has_magic();
}
inline ::google::protobuf::int64 Index::magic() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.magic)
  return magic_;
}
inline void Index::set_magic(::google::protobuf::int64 value) {
  set_has_magic();
  magic_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Index.magic)
}

// optional int64 version = 2;
inline bool Index::has_version() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Index::set_has_version() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Index::clear_has_version() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Index::clear_version() {
  version_ = GOOGLE_LONGLONG(0);
  clear_has_version();
}
inline ::google::protobuf::int64 Index::version() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.version)
  return version_;
}
inline void Index::set_version(::google::protobuf::int64 value) {
  set_has_version();
  version_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Index.version)
}

// optional int64 fs_id = 3;
inline bool Index::has_fs_id() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Index::set_has_fs_id() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Index::clear_has_fs_id() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Index::clear_fs_id() {
  fs_id_ = GOOGLE_LONGLONG(0);
  clear_has_fs_id();
}
inline ::google::protobuf::int64 Index::fs_id() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.fs_id)
  return fs_id_;
}
inline void Index::set_fs_id(::google::protobuf::int64 value) {
  set_has_fs_id();
  fs_id_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Index.fs_id)
}

// optional int64 first_xid = 4;
inline bool Index::has_first_xid() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void Index::set_has_first_xid() {
  _has_bits_[0] |= 0x00000008u;
}
inline void Index::clear_has_first_xid() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void Index::clear_first_xid() {
  first_xid_ = GOOGLE_LONGLONG(0);
  clear_has_first_xid();
}
inline ::google::protobuf::int64 Index::first_xid() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.first_xid)
  return first_xid_;
}
inline void Index::set_first_xid(::google::protobuf::int64 value) {
  set_has_first_xid();
  first_xid_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Index.first_xid)
}

// optional int64 last_xid = 5;
inline bool Index::has_last_xid() const {
  return (_has_bits_[0] & 0x00000010u) != 0;
}
inline void Index::set_has_last_xid() {
  _has_bits_[0] |= 0x00000010u;
}
inline void Index::clear_has_last_xid() {
  _has_bits_[0] &= ~0x00000010u;
}
inline void Index::clear_last_xid() {
  last_xid_ = GOOGLE_LONGLONG(0);
  clear_has_last_xid();
}
inline ::google::protobuf::int64 Index::last_xid() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.last_xid)
  return last_xid_;
}
inline void Index::set_last_xid(::google::protobuf::int64 value) {
  set_has_last_xid();
  last_xid_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Index.last_xid)
}

// repeated .pqfs.proto.Segment segment = 6;
inline int Index::segment_size() const {
  return segment_.size();
}
inline void Index::clear_segment() {
  segment_.Clear();
}
inline const ::pqfs::proto::Segment& Index::segment(int index) const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Index.segment)
  return segment_.Get(index);
}
inline ::pqfs::proto::Segment* Index::mutable_segment(int index) {
  // @@protoc_insertion_point(field_mutable:pqfs.proto.Index.segment)
  return segment_.Mutable(index);
}
inline ::pqfs::proto::Segment* Index::add_segment() {
  // @@protoc_insertion_point(field_add:pqfs.proto.Index.segment)
  return segment_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::pqfs::proto::Segment >&
Index::segment() const {
  // @@protoc_insertion_point(field_list:pqfs.proto.Index.segment)
  return segment_;
}
inline ::google::protobuf::RepeatedPtrField< ::pqfs::proto::Segment >*
Index::mutable_segment() {
  // @@protoc_insertion_point(field_mutable_list:pqfs.proto.Index.segment)
  return &segment_;
}

// -------------------------------------------------------------------

// InodeOffset

// optional int64 inode = 1;
inline bool InodeOffset::has_inode() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void InodeOffset::set_has_inode() {
  _has_bits_[0] |= 0x00000001u;
}
inline void InodeOffset::clear_has_inode() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void InodeOffset::clear_inode() {
  inode_ = GOOGLE_LONGLONG(0);
  clear_has_inode();
}
inline ::google::protobuf::int64 InodeOffset::inode() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.InodeOffset.inode)
  return inode_;
}
inline void InodeOffset::set_inode(::google::protobuf::int64 value) {
  set_has_inode();
  inode_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.InodeOffset.inode)
}

// optional int64 offset = 2;
inline bool InodeOffset::has_offset() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void InodeOffset::set_has_offset() {
  _has_bits_[0] |= 0x00000002u;
}
inline void InodeOffset::clear_has_offset() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void InodeOffset::clear_offset() {
  offset_ = GOOGLE_LONGLONG(0);
  clear_has_offset();
}
inline ::google::protobuf::int64 InodeOffset::offset() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.InodeOffset.offset)
  return offset_;
}
inline void InodeOffset::set_offset(::google::protobuf::int64 value) {
  set_has_offset();
  offset_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.InodeOffset.offset)
}

// optional string pathname = 3;
inline bool InodeOffset::has_pathname() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void InodeOffset::set_has_pathname() {
  _has_bits_[0] |= 0x00000004u;
}
inline void InodeOffset::clear_has_pathname() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void InodeOffset::clear_pathname() {
  if (pathname_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    pathname_->clear();
  }
  clear_has_pathname();
}
inline const ::std::string& InodeOffset::pathname() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.InodeOffset.pathname)
  return *pathname_;
}
inline void InodeOffset::set_pathname(const ::std::string& value) {
  set_has_pathname();
  if (pathname_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    pathname_ = new ::std::string;
  }
  pathname_->assign(value);
  // @@protoc_insertion_point(field_set:pqfs.proto.InodeOffset.pathname)
}
inline void InodeOffset::set_pathname(const char* value) {
  set_has_pathname();
  if (pathname_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    pathname_ = new ::std::string;
  }
  pathname_->assign(value);
  // @@protoc_insertion_point(field_set_char:pqfs.proto.InodeOffset.pathname)
}
inline void InodeOffset::set_pathname(const char* value, size_t size) {
  set_has_pathname();
  if (pathname_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    pathname_ = new ::std::string;
  }
  pathname_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:pqfs.proto.InodeOffset.pathname)
}
inline ::std::string* InodeOffset::mutable_pathname() {
  set_has_pathname();
  if (pathname_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    pathname_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:pqfs.proto.InodeOffset.pathname)
  return pathname_;
}
inline ::std::string* InodeOffset::release_pathname() {
  clear_has_pathname();
  if (pathname_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = pathname_;
    pathname_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void InodeOffset::set_allocated_pathname(::std::string* pathname) {
  if (pathname_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete pathname_;
  }
  if (pathname) {
    set_has_pathname();
    pathname_ = pathname;
  } else {
    clear_has_pathname();
    pathname_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:pqfs.proto.InodeOffset.pathname)
}

// -------------------------------------------------------------------

// Segment

// optional string name = 1;
inline bool Segment::has_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Segment::set_has_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Segment::clear_has_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Segment::clear_name() {
  if (name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    name_->clear();
  }
  clear_has_name();
}
inline const ::std::string& Segment::name() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Segment.name)
  return *name_;
}
inline void Segment::set_name(const ::std::string& value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    name_ = new ::std::string;
  }
  name_->assign(value);
  // @@protoc_insertion_point(field_set:pqfs.proto.Segment.name)
}
inline void Segment::set_name(const char* value) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    name_ = new ::std::string;
  }
  name_->assign(value);
  // @@protoc_insertion_point(field_set_char:pqfs.proto.Segment.name)
}
inline void Segment::set_name(const char* value, size_t size) {
  set_has_name();
  if (name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    name_ = new ::std::string;
  }
  name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:pqfs.proto.Segment.name)
}
inline ::std::string* Segment::mutable_name() {
  set_has_name();
  if (name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:pqfs.proto.Segment.name)
  return name_;
}
inline ::std::string* Segment::release_name() {
  clear_has_name();
  if (name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = name_;
    name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void Segment::set_allocated_name(::std::string* name) {
  if (name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete name_;
  }
  if (name) {
    set_has_name();
    name_ = name;
  } else {
    clear_has_name();
    name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:pqfs.proto.Segment.name)
}

// optional int64 checksum = 2;
inline bool Segment::has_checksum() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Segment::set_has_checksum() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Segment::clear_has_checksum() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Segment::clear_checksum() {
  checksum_ = GOOGLE_LONGLONG(0);
  clear_has_checksum();
}
inline ::google::protobuf::int64 Segment::checksum() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Segment.checksum)
  return checksum_;
}
inline void Segment::set_checksum(::google::protobuf::int64 value) {
  set_has_checksum();
  checksum_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Segment.checksum)
}

// optional int64 first_xid = 3;
inline bool Segment::has_first_xid() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Segment::set_has_first_xid() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Segment::clear_has_first_xid() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Segment::clear_first_xid() {
  first_xid_ = GOOGLE_LONGLONG(0);
  clear_has_first_xid();
}
inline ::google::protobuf::int64 Segment::first_xid() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Segment.first_xid)
  return first_xid_;
}
inline void Segment::set_first_xid(::google::protobuf::int64 value) {
  set_has_first_xid();
  first_xid_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Segment.first_xid)
}

// optional int64 last_xid = 4;
inline bool Segment::has_last_xid() const {
  return (_has_bits_[0] & 0x00000008u) != 0;
}
inline void Segment::set_has_last_xid() {
  _has_bits_[0] |= 0x00000008u;
}
inline void Segment::clear_has_last_xid() {
  _has_bits_[0] &= ~0x00000008u;
}
inline void Segment::clear_last_xid() {
  last_xid_ = GOOGLE_LONGLONG(0);
  clear_has_last_xid();
}
inline ::google::protobuf::int64 Segment::last_xid() const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Segment.last_xid)
  return last_xid_;
}
inline void Segment::set_last_xid(::google::protobuf::int64 value) {
  set_has_last_xid();
  last_xid_ = value;
  // @@protoc_insertion_point(field_set:pqfs.proto.Segment.last_xid)
}

// repeated .pqfs.proto.InodeOffset inode_offset = 5;
inline int Segment::inode_offset_size() const {
  return inode_offset_.size();
}
inline void Segment::clear_inode_offset() {
  inode_offset_.Clear();
}
inline const ::pqfs::proto::InodeOffset& Segment::inode_offset(int index) const {
  // @@protoc_insertion_point(field_get:pqfs.proto.Segment.inode_offset)
  return inode_offset_.Get(index);
}
inline ::pqfs::proto::InodeOffset* Segment::mutable_inode_offset(int index) {
  // @@protoc_insertion_point(field_mutable:pqfs.proto.Segment.inode_offset)
  return inode_offset_.Mutable(index);
}
inline ::pqfs::proto::InodeOffset* Segment::add_inode_offset() {
  // @@protoc_insertion_point(field_add:pqfs.proto.Segment.inode_offset)
  return inode_offset_.Add();
}
inline const ::google::protobuf::RepeatedPtrField< ::pqfs::proto::InodeOffset >&
Segment::inode_offset() const {
  // @@protoc_insertion_point(field_list:pqfs.proto.Segment.inode_offset)
  return inode_offset_;
}
inline ::google::protobuf::RepeatedPtrField< ::pqfs::proto::InodeOffset >*
Segment::mutable_inode_offset() {
  // @@protoc_insertion_point(field_mutable_list:pqfs.proto.Segment.inode_offset)
  return &inode_offset_;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace pqfs

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_index_2eproto__INCLUDED
