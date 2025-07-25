#pragma once

#include "root.h"
#include "wtf/text/ASCIILiteral.h"

#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/Exception.h>
#include <JavaScriptCore/Identifier.h>
#include <JavaScriptCore/JSGlobalObject.h>
#include <JavaScriptCore/JSString.h>
#include <JavaScriptCore/ThrowScope.h>
#include <JavaScriptCore/VM.h>
#include <limits>

namespace Zig {
class GlobalObject;
}

#include "headers-handwritten.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

extern "C" size_t Bun__stringSyntheticAllocationLimit;
extern "C" const char* Bun__errnoName(int);

namespace Zig {

// 8 bit byte
// we tag the final two bits
// so 56 bits are copied over
// rest we zero out for consistentcy
static const unsigned char* untag(const unsigned char* ptr)
{
    return reinterpret_cast<const unsigned char*>(
        (((reinterpret_cast<uintptr_t>(ptr) & ~(static_cast<uint64_t>(1) << 63) & ~(static_cast<uint64_t>(1) << 62)) & ~(static_cast<uint64_t>(1) << 61)) & ~(static_cast<uint64_t>(1) << 60)));
}

static void* untagVoid(const unsigned char* ptr)
{
    return const_cast<void*>(reinterpret_cast<const void*>(untag(ptr)));
}

static void* untagVoid(const char16_t* ptr)
{
    return untagVoid(reinterpret_cast<const unsigned char*>(ptr));
}

static bool isTaggedUTF16Ptr(const unsigned char* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & (static_cast<uint64_t>(1) << 63)) != 0;
}

// Do we need to convert the string from UTF-8 to UTF-16?
static bool isTaggedUTF8Ptr(const unsigned char* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & (static_cast<uint64_t>(1) << 61)) != 0;
}

static bool isTaggedExternalPtr(const unsigned char* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & (static_cast<uint64_t>(1) << 62)) != 0;
}

static void free_global_string(void* str, void* ptr, unsigned len)
{
    // i don't understand why this happens
    if (ptr == nullptr)
        return;

    ZigString__freeGlobal(reinterpret_cast<const unsigned char*>(ptr), len);
}

// Switching to AtomString doesn't yield a perf benefit because we're recreating it each time.
static const WTF::String toString(ZigString str)
{
    if (str.len == 0 || str.ptr == nullptr) {
        return WTF::String();
    }
    if (isTaggedUTF8Ptr(str.ptr)) [[unlikely]] {
        ASSERT_WITH_MESSAGE(!isTaggedExternalPtr(str.ptr), "UTF8 and external ptr are mutually exclusive. The external will never be freed.");
        return WTF::String::fromUTF8ReplacingInvalidSequences(std::span { untag(str.ptr), str.len });
    }

    if (isTaggedExternalPtr(str.ptr)) [[unlikely]] {
        // This will fail if the string is too long. Let's make it explicit instead of an ASSERT.
        if (str.len > Bun__stringSyntheticAllocationLimit) [[unlikely]] {
            free_global_string(nullptr, reinterpret_cast<void*>(const_cast<unsigned char*>(untag(str.ptr))), static_cast<unsigned>(str.len));
            return {};
        }

        return !isTaggedUTF16Ptr(str.ptr)
            ? WTF::String(WTF::ExternalStringImpl::create({ untag(str.ptr), str.len }, untagVoid(str.ptr), free_global_string))
            : WTF::String(WTF::ExternalStringImpl::create(
                  { reinterpret_cast<const char16_t*>(untag(str.ptr)), str.len }, untagVoid(str.ptr), free_global_string));
    }

    // This will fail if the string is too long. Let's make it explicit instead of an ASSERT.
    if (str.len > Bun__stringSyntheticAllocationLimit) [[unlikely]] {
        return {};
    }

    return !isTaggedUTF16Ptr(str.ptr)
        ? WTF::String(WTF::StringImpl::createWithoutCopying({ untag(str.ptr), str.len }))
        : WTF::String(WTF::StringImpl::createWithoutCopying(
              { reinterpret_cast<const char16_t*>(untag(str.ptr)), str.len }));
}

static WTF::AtomString toAtomString(ZigString str)
{

    if (!isTaggedUTF16Ptr(str.ptr)) {
        return makeAtomString(untag(str.ptr), str.len);
    } else {
        return makeAtomString(reinterpret_cast<const char16_t*>(untag(str.ptr)), str.len);
    }
}

static const WTF::String toString(ZigString str, StringPointer ptr)
{
    if (str.len == 0 || str.ptr == nullptr || ptr.len == 0) {
        return WTF::String();
    }
    if (isTaggedUTF8Ptr(str.ptr)) [[unlikely]] {
        return WTF::String::fromUTF8ReplacingInvalidSequences(std::span { &untag(str.ptr)[ptr.off], ptr.len });
    }

    // This will fail if the string is too long. Let's make it explicit instead of an ASSERT.
    if (str.len > Bun__stringSyntheticAllocationLimit) [[unlikely]] {
        return {};
    }

    return !isTaggedUTF16Ptr(str.ptr)
        ? WTF::String(WTF::StringImpl::createWithoutCopying({ &untag(str.ptr)[ptr.off], ptr.len }))
        : WTF::String(WTF::StringImpl::createWithoutCopying(
              { &reinterpret_cast<const char16_t*>(untag(str.ptr))[ptr.off], ptr.len }));
}

static const WTF::String toStringCopy(ZigString str, StringPointer ptr)
{
    if (str.len == 0 || str.ptr == nullptr || ptr.len == 0) {
        return WTF::String();
    }
    if (isTaggedUTF8Ptr(str.ptr)) [[unlikely]] {
        return WTF::String::fromUTF8ReplacingInvalidSequences(std::span { &untag(str.ptr)[ptr.off], ptr.len });
    }

    // This will fail if the string is too long. Let's make it explicit instead of an ASSERT.
    if (str.len > Bun__stringSyntheticAllocationLimit) [[unlikely]] {
        return {};
    }

    return !isTaggedUTF16Ptr(str.ptr)
        ? WTF::String(WTF::StringImpl::create(std::span { &untag(str.ptr)[ptr.off], ptr.len }))
        : WTF::String(WTF::StringImpl::create(
              std::span { &reinterpret_cast<const char16_t*>(untag(str.ptr))[ptr.off], ptr.len }));
}

static const WTF::String toStringCopy(ZigString str)
{
    if (str.len == 0 || str.ptr == nullptr) {
        return WTF::String();
    }
    if (isTaggedUTF8Ptr(str.ptr)) [[unlikely]] {
        return WTF::String::fromUTF8ReplacingInvalidSequences(std::span { untag(str.ptr), str.len });
    }

    if (isTaggedUTF16Ptr(str.ptr)) {
        std::span<char16_t> out;
        auto impl = WTF::StringImpl::tryCreateUninitialized(str.len, out);
        if (!impl) [[unlikely]] {
            return WTF::String();
        }
        memcpy(out.data(), untag(str.ptr), str.len * sizeof(char16_t));
        return WTF::String(WTFMove(impl));
    } else {
        std::span<LChar> out;
        auto impl = WTF::StringImpl::tryCreateUninitialized(str.len, out);
        if (!impl) [[unlikely]]
            return WTF::String();
        memcpy(out.data(), untag(str.ptr), str.len * sizeof(LChar));
        return WTF::String(WTFMove(impl));
    }
}

static WTF::String toStringNotConst(ZigString str) { return toString(str); }

static const JSC::JSString* toJSString(ZigString str, JSC::JSGlobalObject* global)
{
    return JSC::jsOwnedString(global->vm(), toString(str));
}

static JSC::JSString* toJSStringGC(ZigString str, JSC::JSGlobalObject* global)
{
    return JSC::jsString(global->vm(), toStringCopy(str));
}

static const ZigString ZigStringEmpty = ZigString { (unsigned char*)"", 0 };
static const unsigned char __dot_char = '.';
static const ZigString ZigStringCwd = ZigString { &__dot_char, 1 };
static const BunString BunStringCwd = BunString { BunStringTag::StaticZigString, ZigStringCwd };
static const BunString BunStringEmpty = BunString { BunStringTag::Empty, nullptr };

static const unsigned char* taggedUTF16Ptr(const char16_t* ptr)
{
    return reinterpret_cast<const unsigned char*>(reinterpret_cast<uintptr_t>(ptr) | (static_cast<uint64_t>(1) << 63));
}

static ZigString toZigString(WTF::String* str)
{
    return str->isEmpty()
        ? ZigStringEmpty
        : ZigString { str->is8Bit() ? str->span8().data() : taggedUTF16Ptr(str->span16().data()),
              str->length() };
}

static ZigString toZigString(WTF::StringImpl& str)
{
    return str.isEmpty()
        ? ZigStringEmpty
        : ZigString { str.is8Bit() ? str.span8().data() : taggedUTF16Ptr(str.span16().data()),
              str.length() };
}

static ZigString toZigString(WTF::StringView& str)
{
    return str.isEmpty()
        ? ZigStringEmpty
        : ZigString { str.is8Bit() ? str.span8().data() : taggedUTF16Ptr(str.span16().data()),
              str.length() };
}

static ZigString toZigString(const WTF::StringView& str)
{
    return str.isEmpty()
        ? ZigStringEmpty
        : ZigString { str.is8Bit() ? str.span8().data() : taggedUTF16Ptr(str.span16().data()),
              str.length() };
}

static ZigString toZigString(JSC::JSString& str, JSC::JSGlobalObject* global)
{
    if (str.isSubstring()) {
        return toZigString(str.view(global));
    }

    return toZigString(str.value(global));
}

static ZigString toZigString(JSC::JSString* str, JSC::JSGlobalObject* global)
{
    if (str->isSubstring()) {
        return toZigString(str->view(global));
    }
    return toZigString(str->value(global));
}

static ZigString toZigString(JSC::Identifier& str, JSC::JSGlobalObject* global)
{
    return toZigString(str.string());
}

static ZigString toZigString(JSC::Identifier* str, JSC::JSGlobalObject* global)
{
    return toZigString(str->string());
}

static WTF::StringView toStringView(ZigString str)
{
    return WTF::StringView(std::span { untag(str.ptr), str.len });
}

static void throwException(JSC::ThrowScope& scope, ZigErrorType err, JSC::JSGlobalObject* global)
{
    scope.throwException(global,
        JSC::Exception::create(global->vm(), JSC::JSValue::decode(err.value)));
}

static ZigString toZigString(JSC::JSValue val, JSC::JSGlobalObject* global)
{
    auto scope = DECLARE_THROW_SCOPE(global->vm());
    auto* str = val.toString(global);

    if (scope.exception()) [[unlikely]] {
        scope.clearException();
        scope.release();
        return ZigStringEmpty;
    }

    auto view = str->view(global);
    if (scope.exception()) [[unlikely]] {
        scope.clearException();
        scope.release();
        return ZigStringEmpty;
    }

    return toZigString(view);
}

static const WTF::String toStringStatic(ZigString str)
{
    if (str.len == 0 || str.ptr == nullptr) {
        return WTF::String();
    }
    if (isTaggedUTF8Ptr(str.ptr)) [[unlikely]] {
        abort();
    }

    if (isTaggedUTF16Ptr(str.ptr)) {
        return WTF::String(AtomStringImpl::add(std::span { reinterpret_cast<const char16_t*>(untag(str.ptr)), str.len }));
    }

    auto* untagged = untag(str.ptr);
    ASSERT(untagged[str.len] == 0);
    ASCIILiteral ascii = ASCIILiteral::fromLiteralUnsafe(reinterpret_cast<const char*>(untagged));
    return WTF::String(ascii);
}

static JSC::JSValue getErrorInstance(const ZigString* str, JSC::JSGlobalObject* globalObject)
{
    WTF::String message = toString(*str);
    if (message.isNull() && str->len > 0) [[unlikely]] {
        // pending exception while creating an error.
        return {};
    }

    JSC::JSObject* result = JSC::createError(globalObject, message);
    JSC::EnsureStillAliveScope ensureAlive(result);

    return result;
}

static JSC::JSValue getTypeErrorInstance(const ZigString* str, JSC::JSGlobalObject* globalObject)
{
    JSC::JSObject* result = JSC::createTypeError(globalObject, toStringCopy(*str));
    JSC::EnsureStillAliveScope ensureAlive(result);

    return result;
}

static JSC::JSValue getSyntaxErrorInstance(const ZigString* str, JSC::JSGlobalObject* globalObject)
{
    JSC::JSObject* result = JSC::createSyntaxError(globalObject, toStringCopy(*str));
    JSC::EnsureStillAliveScope ensureAlive(result);

    return result;
}

static JSC::JSValue getRangeErrorInstance(const ZigString* str, JSC::JSGlobalObject* globalObject)
{
    JSC::JSObject* result = JSC::createRangeError(globalObject, toStringCopy(*str));
    JSC::EnsureStillAliveScope ensureAlive(result);

    return result;
}

static const JSC::Identifier toIdentifier(ZigString str, JSC::JSGlobalObject* global)
{
    if (str.len == 0 || str.ptr == nullptr) {
        return JSC::Identifier::EmptyIdentifier;
    }
    WTF::String wtfstr = Zig::isTaggedExternalPtr(str.ptr) ? toString(str) : Zig::toStringCopy(str);
    JSC::Identifier id = JSC::Identifier::fromString(global->vm(), wtfstr);
    return id;
}

}; // namespace Zig

JSC::JSValue createSystemError(JSC::JSGlobalObject* global, ASCIILiteral message, ASCIILiteral syscall, int err);
JSC::JSValue createSystemError(JSC::JSGlobalObject* global, ASCIILiteral syscall, int err);

static void throwSystemError(JSC::ThrowScope& scope, JSC::JSGlobalObject* globalObject, ASCIILiteral syscall, int err)
{
    scope.throwException(globalObject, createSystemError(globalObject, syscall, err));
}

static void throwSystemError(JSC::ThrowScope& scope, JSC::JSGlobalObject* globalObject, ASCIILiteral message, ASCIILiteral syscall, int err)
{
    scope.throwException(globalObject, createSystemError(globalObject, message, syscall, err));
}

template<typename WebCoreType, typename OutType>
OutType* WebCoreCast(JSC::EncodedJSValue JSValue0)
{
    // we must use jsDynamicCast here so that we check that the type is correct
    WebCoreType* jsdomURL = JSC::jsDynamicCast<WebCoreType*>(JSC::JSValue::decode(JSValue0));
    if (jsdomURL == nullptr) {
        return nullptr;
    }

    return reinterpret_cast<OutType*>(&jsdomURL->wrapped());
}

#pragma clang diagnostic pop
