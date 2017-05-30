// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: super.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "super.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace pqfs {
namespace proto {

namespace {

const ::google::protobuf::Descriptor* Superblock_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  Superblock_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_super_2eproto() {
  protobuf_AddDesc_super_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "super.proto");
  GOOGLE_CHECK(file != NULL);
  Superblock_descriptor_ = file->message_type(0);
  static const int Superblock_offsets_[8] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, magic_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, filesystem_name_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, cloud_uri_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, next_inode_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, next_xid_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, fs_id_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, dev_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, chunk_size_),
  };
  Superblock_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      Superblock_descriptor_,
      Superblock::default_instance_,
      Superblock_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(Superblock, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(Superblock));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_super_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    Superblock_descriptor_, &Superblock::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_super_2eproto() {
  delete Superblock::default_instance_;
  delete Superblock_reflection_;
}

void protobuf_AddDesc_super_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\013super.proto\022\npqfs.proto\"\235\001\n\nSuperblock"
    "\022\r\n\005magic\030\001 \001(\003\022\027\n\017filesystem_name\030\002 \001(\t"
    "\022\021\n\tcloud_uri\030\003 \001(\t\022\022\n\nnext_inode\030\004 \001(\003\022"
    "\020\n\010next_xid\030\005 \001(\003\022\r\n\005fs_id\030\006 \001(\003\022\013\n\003dev\030"
    "\007 \001(\003\022\022\n\nchunk_size\030\010 \001(\003", 185);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "super.proto", &protobuf_RegisterTypes);
  Superblock::default_instance_ = new Superblock();
  Superblock::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_super_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_super_2eproto {
  StaticDescriptorInitializer_super_2eproto() {
    protobuf_AddDesc_super_2eproto();
  }
} static_descriptor_initializer_super_2eproto_;

// ===================================================================

#ifndef _MSC_VER
const int Superblock::kMagicFieldNumber;
const int Superblock::kFilesystemNameFieldNumber;
const int Superblock::kCloudUriFieldNumber;
const int Superblock::kNextInodeFieldNumber;
const int Superblock::kNextXidFieldNumber;
const int Superblock::kFsIdFieldNumber;
const int Superblock::kDevFieldNumber;
const int Superblock::kChunkSizeFieldNumber;
#endif  // !_MSC_VER

Superblock::Superblock()
  : ::google::protobuf::Message() {
  SharedCtor();
  // @@protoc_insertion_point(constructor:pqfs.proto.Superblock)
}

void Superblock::InitAsDefaultInstance() {
}

Superblock::Superblock(const Superblock& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:pqfs.proto.Superblock)
}

void Superblock::SharedCtor() {
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  magic_ = GOOGLE_LONGLONG(0);
  filesystem_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  cloud_uri_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  next_inode_ = GOOGLE_LONGLONG(0);
  next_xid_ = GOOGLE_LONGLONG(0);
  fs_id_ = GOOGLE_LONGLONG(0);
  dev_ = GOOGLE_LONGLONG(0);
  chunk_size_ = GOOGLE_LONGLONG(0);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

Superblock::~Superblock() {
  // @@protoc_insertion_point(destructor:pqfs.proto.Superblock)
  SharedDtor();
}

void Superblock::SharedDtor() {
  if (filesystem_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete filesystem_name_;
  }
  if (cloud_uri_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete cloud_uri_;
  }
  if (this != default_instance_) {
  }
}

void Superblock::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* Superblock::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return Superblock_descriptor_;
}

const Superblock& Superblock::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_super_2eproto();
  return *default_instance_;
}

Superblock* Superblock::default_instance_ = NULL;

Superblock* Superblock::New() const {
  return new Superblock;
}

void Superblock::Clear() {
#define OFFSET_OF_FIELD_(f) (reinterpret_cast<char*>(      \
  &reinterpret_cast<Superblock*>(16)->f) - \
   reinterpret_cast<char*>(16))

#define ZR_(first, last) do {                              \
    size_t f = OFFSET_OF_FIELD_(first);                    \
    size_t n = OFFSET_OF_FIELD_(last) - f + sizeof(last);  \
    ::memset(&first, 0, n);                                \
  } while (0)

  if (_has_bits_[0 / 32] & 255) {
    ZR_(next_inode_, chunk_size_);
    magic_ = GOOGLE_LONGLONG(0);
    if (has_filesystem_name()) {
      if (filesystem_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        filesystem_name_->clear();
      }
    }
    if (has_cloud_uri()) {
      if (cloud_uri_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
        cloud_uri_->clear();
      }
    }
  }

#undef OFFSET_OF_FIELD_
#undef ZR_

  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool Superblock::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:pqfs.proto.Superblock)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional int64 magic = 1;
      case 1: {
        if (tag == 8) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &magic_)));
          set_has_magic();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_filesystem_name;
        break;
      }

      // optional string filesystem_name = 2;
      case 2: {
        if (tag == 18) {
         parse_filesystem_name:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_filesystem_name()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->filesystem_name().data(), this->filesystem_name().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "filesystem_name");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(26)) goto parse_cloud_uri;
        break;
      }

      // optional string cloud_uri = 3;
      case 3: {
        if (tag == 26) {
         parse_cloud_uri:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_cloud_uri()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
            this->cloud_uri().data(), this->cloud_uri().length(),
            ::google::protobuf::internal::WireFormat::PARSE,
            "cloud_uri");
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(32)) goto parse_next_inode;
        break;
      }

      // optional int64 next_inode = 4;
      case 4: {
        if (tag == 32) {
         parse_next_inode:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &next_inode_)));
          set_has_next_inode();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(40)) goto parse_next_xid;
        break;
      }

      // optional int64 next_xid = 5;
      case 5: {
        if (tag == 40) {
         parse_next_xid:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &next_xid_)));
          set_has_next_xid();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(48)) goto parse_fs_id;
        break;
      }

      // optional int64 fs_id = 6;
      case 6: {
        if (tag == 48) {
         parse_fs_id:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &fs_id_)));
          set_has_fs_id();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(56)) goto parse_dev;
        break;
      }

      // optional int64 dev = 7;
      case 7: {
        if (tag == 56) {
         parse_dev:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &dev_)));
          set_has_dev();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(64)) goto parse_chunk_size;
        break;
      }

      // optional int64 chunk_size = 8;
      case 8: {
        if (tag == 64) {
         parse_chunk_size:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::int64, ::google::protobuf::internal::WireFormatLite::TYPE_INT64>(
                 input, &chunk_size_)));
          set_has_chunk_size();
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:pqfs.proto.Superblock)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:pqfs.proto.Superblock)
  return false;
#undef DO_
}

void Superblock::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:pqfs.proto.Superblock)
  // optional int64 magic = 1;
  if (has_magic()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(1, this->magic(), output);
  }

  // optional string filesystem_name = 2;
  if (has_filesystem_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->filesystem_name().data(), this->filesystem_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "filesystem_name");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      2, this->filesystem_name(), output);
  }

  // optional string cloud_uri = 3;
  if (has_cloud_uri()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->cloud_uri().data(), this->cloud_uri().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "cloud_uri");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      3, this->cloud_uri(), output);
  }

  // optional int64 next_inode = 4;
  if (has_next_inode()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(4, this->next_inode(), output);
  }

  // optional int64 next_xid = 5;
  if (has_next_xid()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(5, this->next_xid(), output);
  }

  // optional int64 fs_id = 6;
  if (has_fs_id()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(6, this->fs_id(), output);
  }

  // optional int64 dev = 7;
  if (has_dev()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(7, this->dev(), output);
  }

  // optional int64 chunk_size = 8;
  if (has_chunk_size()) {
    ::google::protobuf::internal::WireFormatLite::WriteInt64(8, this->chunk_size(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
  // @@protoc_insertion_point(serialize_end:pqfs.proto.Superblock)
}

::google::protobuf::uint8* Superblock::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:pqfs.proto.Superblock)
  // optional int64 magic = 1;
  if (has_magic()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(1, this->magic(), target);
  }

  // optional string filesystem_name = 2;
  if (has_filesystem_name()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->filesystem_name().data(), this->filesystem_name().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "filesystem_name");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        2, this->filesystem_name(), target);
  }

  // optional string cloud_uri = 3;
  if (has_cloud_uri()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8StringNamedField(
      this->cloud_uri().data(), this->cloud_uri().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE,
      "cloud_uri");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        3, this->cloud_uri(), target);
  }

  // optional int64 next_inode = 4;
  if (has_next_inode()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(4, this->next_inode(), target);
  }

  // optional int64 next_xid = 5;
  if (has_next_xid()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(5, this->next_xid(), target);
  }

  // optional int64 fs_id = 6;
  if (has_fs_id()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(6, this->fs_id(), target);
  }

  // optional int64 dev = 7;
  if (has_dev()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(7, this->dev(), target);
  }

  // optional int64 chunk_size = 8;
  if (has_chunk_size()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteInt64ToArray(8, this->chunk_size(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  // @@protoc_insertion_point(serialize_to_array_end:pqfs.proto.Superblock)
  return target;
}

int Superblock::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional int64 magic = 1;
    if (has_magic()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->magic());
    }

    // optional string filesystem_name = 2;
    if (has_filesystem_name()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->filesystem_name());
    }

    // optional string cloud_uri = 3;
    if (has_cloud_uri()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->cloud_uri());
    }

    // optional int64 next_inode = 4;
    if (has_next_inode()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->next_inode());
    }

    // optional int64 next_xid = 5;
    if (has_next_xid()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->next_xid());
    }

    // optional int64 fs_id = 6;
    if (has_fs_id()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->fs_id());
    }

    // optional int64 dev = 7;
    if (has_dev()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->dev());
    }

    // optional int64 chunk_size = 8;
    if (has_chunk_size()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::Int64Size(
          this->chunk_size());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void Superblock::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const Superblock* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const Superblock*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void Superblock::MergeFrom(const Superblock& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_magic()) {
      set_magic(from.magic());
    }
    if (from.has_filesystem_name()) {
      set_filesystem_name(from.filesystem_name());
    }
    if (from.has_cloud_uri()) {
      set_cloud_uri(from.cloud_uri());
    }
    if (from.has_next_inode()) {
      set_next_inode(from.next_inode());
    }
    if (from.has_next_xid()) {
      set_next_xid(from.next_xid());
    }
    if (from.has_fs_id()) {
      set_fs_id(from.fs_id());
    }
    if (from.has_dev()) {
      set_dev(from.dev());
    }
    if (from.has_chunk_size()) {
      set_chunk_size(from.chunk_size());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void Superblock::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Superblock::CopyFrom(const Superblock& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Superblock::IsInitialized() const {

  return true;
}

void Superblock::Swap(Superblock* other) {
  if (other != this) {
    std::swap(magic_, other->magic_);
    std::swap(filesystem_name_, other->filesystem_name_);
    std::swap(cloud_uri_, other->cloud_uri_);
    std::swap(next_inode_, other->next_inode_);
    std::swap(next_xid_, other->next_xid_);
    std::swap(fs_id_, other->fs_id_);
    std::swap(dev_, other->dev_);
    std::swap(chunk_size_, other->chunk_size_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata Superblock::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = Superblock_descriptor_;
  metadata.reflection = Superblock_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace pqfs

// @@protoc_insertion_point(global_scope)