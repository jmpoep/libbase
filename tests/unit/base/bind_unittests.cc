#include "base/bind.h"

#include "base/sequenced_task_runner_helpers.h"
#include "base/threading/thread.h"

#include "gtest/gtest.h"

namespace {

int global_int_arg = -1;

struct Counter {
  void add(int x) { sum += x; }

  int sum = 0;
};

void SetGlobalIntArg(int x) {
  global_int_arg = x;
}

void SetGlobalIntFromUniquePtrArg(std::unique_ptr<int> x) {
  global_int_arg = *x;
}

int add(int x, int y) {
  return x + y;
}

TEST(BindTest, FreeFunc) {
  global_int_arg = -1;

  auto cb = base::BindRepeating(&SetGlobalIntArg);
  cb.Run(11);
  EXPECT_EQ(global_int_arg, 11);
  cb.Run(12);
  EXPECT_EQ(global_int_arg, 12);
}

TEST(BindTest, Lambda) {
  global_int_arg = -1;

  auto cb = base::BindRepeating([](int x) { SetGlobalIntArg(x); });
  cb.Run(11);
  EXPECT_EQ(global_int_arg, 11);
  cb.Run(12);
  EXPECT_EQ(global_int_arg, 12);
}

TEST(BindTest, FreeFuncWithArgs) {
  global_int_arg = -1;

  auto cb = base::BindRepeating(&SetGlobalIntArg, 15);
  cb.Run();
  EXPECT_EQ(global_int_arg, 15);
  global_int_arg = -1;
  cb.Run();
  EXPECT_EQ(global_int_arg, 15);
}

TEST(BindTest, LambdaWithArgs) {
  global_int_arg = -1;

  auto cb = base::BindRepeating([](int x) { SetGlobalIntArg(x); }, 15);
  cb.Run();
  EXPECT_EQ(global_int_arg, 15);
  global_int_arg = -1;
  cb.Run();
  EXPECT_EQ(global_int_arg, 15);
}

TEST(BindTest, ClassMethod) {
  Counter counter;
  auto cb = base::BindRepeating(&Counter::add, &counter);
  ASSERT_EQ(counter.sum, 0);
  cb.Run(7);
  EXPECT_EQ(counter.sum, 7);
  cb.Run(3);
  EXPECT_EQ(counter.sum, 10);
}

TEST(BindTest, ClassMethodWithArgs) {
  Counter counter;
  auto cb = base::BindRepeating(&Counter::add, &counter, 21);
  ASSERT_EQ(counter.sum, 0);
  cb.Run();
  EXPECT_EQ(counter.sum, 21);
  cb.Run();
  EXPECT_EQ(counter.sum, 42);
}

TEST(BindTest, MultiArgFreeFunc) {
  auto cb = base::BindRepeating(&add);
  EXPECT_EQ(cb.Run(11, -3), 8);
  EXPECT_EQ(cb.Run(3, -11), -8);
  EXPECT_EQ(cb.Run(0, -11), -11);
}

TEST(BindTest, MultiArgFreeFuncWithSomeArgs) {
  auto cb = base::BindRepeating(&add, 3);
  EXPECT_EQ(cb.Run(11), 14);
  EXPECT_EQ(cb.Run(-3), 0);
  EXPECT_EQ(cb.Run(0), 3);
}

TEST(BindOnceTest, FreeFunc) {
  global_int_arg = -1;

  auto cb = base::BindOnce(&SetGlobalIntArg);
  std::move(cb).Run(11);
  EXPECT_EQ(global_int_arg, 11);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, Lambda) {
  global_int_arg = -1;

  auto cb = base::BindOnce([](int x) { SetGlobalIntArg(x); });
  std::move(cb).Run(11);
  EXPECT_EQ(global_int_arg, 11);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, FreeFuncWithArgs) {
  global_int_arg = -1;

  auto cb = base::BindOnce(&SetGlobalIntArg, 15);
  std::move(cb).Run();
  EXPECT_EQ(global_int_arg, 15);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, LambdaWithArgs) {
  global_int_arg = -1;

  auto cb = base::BindOnce([](int x) { SetGlobalIntArg(x); }, 15);
  std::move(cb).Run();
  EXPECT_EQ(global_int_arg, 15);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, ClassMethod) {
  Counter counter;
  auto cb = base::BindOnce(&Counter::add, &counter);
  ASSERT_EQ(counter.sum, 0);
  std::move(cb).Run(7);
  EXPECT_EQ(counter.sum, 7);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, ClassMethodWithArgs) {
  Counter counter;
  auto cb = base::BindOnce(&Counter::add, &counter, 21);
  ASSERT_EQ(counter.sum, 0);
  std::move(cb).Run();
  EXPECT_EQ(counter.sum, 21);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, MultiArgFreeFunc) {
  auto cb = base::BindOnce(&add);
  EXPECT_EQ(std::move(cb).Run(11, -3), 8);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, MultiArgFreeFuncWithSomeArgs) {
  auto cb = base::BindOnce(&add, 3);
  EXPECT_EQ(std::move(cb).Run(11), 14);
  EXPECT_FALSE(cb);
}

TEST(BindOnceTest, BindUniquePtrAndMove) {
  global_int_arg = -1;

  auto cb_1 =
      base::BindOnce(&SetGlobalIntFromUniquePtrArg, std::make_unique<int>(3));
  auto cb_2 = std::move(cb_1);
  auto cb_3 = base::BindOnce(std::move(cb_2));

  EXPECT_FALSE(cb_1);
  EXPECT_FALSE(cb_2);
  ASSERT_TRUE(cb_3);

  std::move(cb_3).Run();
  EXPECT_EQ(global_int_arg, 3);
}

TEST(AdvancedBindTest, RepeatingToRepeatingCallbackToFreeFunction) {
  auto cb_1 = base::BindRepeating(&add);
  auto cb_2 = base::BindRepeating(cb_1);
  EXPECT_EQ(cb_1.Run(3, 7), 10);
  EXPECT_EQ(cb_1.Run(3, 7), cb_2.Run(3, 7));

  auto cb_3 = base::BindRepeating(cb_2, 1);
  EXPECT_EQ(cb_3.Run(0), 1);
  EXPECT_EQ(cb_3.Run(-3), -2);

  auto cb_4 = base::BindRepeating(cb_3, 3);
  EXPECT_EQ(cb_4.Run(), 4);
  EXPECT_EQ(cb_4.Run(), 4);
}

TEST(AdvancedBindTest, OnceToRepeatingCallbackToFreeFunction) {
  auto cb_1 = base::BindRepeating(&add);
  auto cb_2 = base::BindOnce(cb_1);
  EXPECT_EQ(cb_1.Run(3, 7), 10);
  EXPECT_EQ(std::move(cb_2).Run(3, 7), 10);
  EXPECT_FALSE(cb_2);

  auto cb_3 = base::BindOnce(cb_1);
  EXPECT_EQ(std::move(cb_3).Run(3, 7), 10);
  EXPECT_TRUE(cb_1);
  EXPECT_FALSE(cb_3);

  auto cb_4 = base::BindOnce(cb_1, 1);
  EXPECT_EQ(std::move(cb_4).Run(3), 4);
  EXPECT_TRUE(cb_1);
  EXPECT_FALSE(cb_4);

  auto cb_5 = base::BindRepeating(cb_1, 1);
  auto cb_6 = base::BindOnce(cb_5, 3);
  EXPECT_EQ(std::move(cb_6).Run(), 4);
  EXPECT_TRUE(cb_5);
  EXPECT_FALSE(cb_6);
}

TEST(AdvancedBindTest, OnceToOnceCallbackToFreeFunction) {
  auto cb_1 = base::BindOnce(&add);
  auto cb_2 = base::BindOnce(std::move(cb_1));
  EXPECT_EQ(std::move(cb_2).Run(3, 7), 10);
  EXPECT_FALSE(cb_1);
  EXPECT_FALSE(cb_2);

  auto cb_3 = base::BindOnce(&add);
  auto cb_4 = base::BindOnce(std::move(cb_3), 1);
  EXPECT_EQ(std::move(cb_4).Run(3), 4);
  EXPECT_FALSE(cb_3);
  EXPECT_FALSE(cb_4);

  auto cb_5 = base::BindOnce(&add);
  auto cb_6 = base::BindOnce(std::move(cb_5), 1);
  auto cb_7 = base::BindOnce(std::move(cb_6), 3);
  EXPECT_EQ(std::move(cb_7).Run(), 4);
  EXPECT_FALSE(cb_5);
  EXPECT_FALSE(cb_6);
  EXPECT_FALSE(cb_7);
}

class MemberFunctionsSimple {
 public:
  void Function01() { value |= (1 << 0); }
  void Function02() const { value |= (1 << 1); }
  void Function03() volatile { value |= (1 << 2); }
  void Function04() const volatile { value |= (1 << 3); }
  mutable int value = 0;
};

TEST(MemberFunctionsExhaustionTest, SimpleFunctions) {
  MemberFunctionsSimple obj;

  base::BindOnce(&MemberFunctionsSimple::Function01, &obj).Run();
  base::BindOnce(&MemberFunctionsSimple::Function02, &obj).Run();
  base::BindOnce(&MemberFunctionsSimple::Function03, &obj).Run();
  base::BindOnce(&MemberFunctionsSimple::Function04, &obj).Run();

  EXPECT_EQ(obj.value, 0b1111);
}

class MemberFunctionsOverloaded {
 public:
  void Function05() & { value |= (1 << 0); }
  void Function06() const& { value |= (1 << 1); }
  void Function07() volatile& { value |= (1 << 2); }
  void Function08() const volatile& { value |= (1 << 3); }
  void Function09() && { value |= (1 << 4); }
  void Function10() const&& { value |= (1 << 5); }
  void Function11() volatile&& { value |= (1 << 6); }
  void Function12() const volatile&& { value |= (1 << 7); }
  mutable int value = 0;
};

TEST(MemberFunctionsExhaustionTest, OverloadedFunctions) {
  MemberFunctionsOverloaded obj;

  base::BindOnce(&MemberFunctionsOverloaded::Function05, &obj).Run();
  base::BindOnce(&MemberFunctionsOverloaded::Function06, &obj).Run();
  base::BindOnce(&MemberFunctionsOverloaded::Function07, &obj).Run();
  base::BindOnce(&MemberFunctionsOverloaded::Function08, &obj).Run();

  // Functions 09-12 are non-bindable to pointers since pointers cannot transfer
  // r-valueness

  EXPECT_EQ(obj.value, 0b0000'1111);
}

class MemberFunctionsNoexcept {
 public:
  void Function13() noexcept { value |= (1 << 0); }
  void Function14() const noexcept { value |= (1 << 1); }
  void Function15() volatile noexcept { value |= (1 << 2); }
  void Function16() const volatile noexcept { value |= (1 << 3); }
  mutable int value = 0;
};

TEST(MemberFunctionsExhaustionTest, NoexceptFunctions) {
  MemberFunctionsNoexcept obj;

  base::BindOnce(&MemberFunctionsNoexcept::Function13, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexcept::Function14, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexcept::Function15, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexcept::Function16, &obj).Run();

  EXPECT_EQ(obj.value, 0b1111);
}

class MemberFunctionsNoexceptOverloaded {
 public:
  void Function17() & noexcept { value |= (1 << 0); }
  void Function18() const& noexcept { value |= (1 << 1); }
  void Function19() volatile& noexcept { value |= (1 << 2); }
  void Function20() const volatile& noexcept { value |= (1 << 3); }
  void Function21() && noexcept { value |= (1 << 4); }
  void Function22() const&& noexcept { value |= (1 << 5); }
  void Function23() volatile&& noexcept { value |= (1 << 6); }
  void Function24() const volatile&& noexcept { value |= (1 << 7); }
  mutable int value = 0;
};

TEST(MemberFunctionsExhaustionTest, NoexceptOverloadedFunctions) {
  MemberFunctionsNoexceptOverloaded obj;

  base::BindOnce(&MemberFunctionsNoexceptOverloaded::Function17, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexceptOverloaded::Function18, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexceptOverloaded::Function19, &obj).Run();
  base::BindOnce(&MemberFunctionsNoexceptOverloaded::Function20, &obj).Run();

  // Functions 21-24 are non-bindable to pointers since pointers cannot transfer
  // r-valueness

  EXPECT_EQ(obj.value, 0b0000'1111);
}

class WeakClass {
 public:
  WeakClass() : value_(0u), weak_factory_(this) {}

  base::WeakPtr<WeakClass> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }
  void InvalidateWeakPtrs() { weak_factory_.InvalidateWeakPtrs(); }

  size_t GetValue() { return value_; }
  void IncrementValue() { ++value_; }
  void IncrementBy(size_t increment_value) { value_ += increment_value; }
  size_t IncrementAndGetValue() { return ++value_; }

  void VerifyExpectation(size_t expected_value) {
    EXPECT_EQ(GetValue(), expected_value);
  }

 private:
  size_t value_;
  base::WeakPtrFactory<WeakClass> weak_factory_;
};

class WeakCallbackTest : public ::testing::Test {
 public:
  WeakCallbackTest()
      : sequence_id_setter_(
            base::detail::SequenceIdGenerator::GetNextSequenceId()) {}

 protected:
  // We need to emulate that we're on some sequence for DCHECKs to work
  // correctly.
  base::detail::ScopedSequenceIdSetter sequence_id_setter_;
  WeakClass weak_object_;
};

TEST_F(WeakCallbackTest, EmptyWeakOnceCallback) {
  base::WeakPtr<WeakClass> weak_object{nullptr};
  auto callback = base::BindOnce(&WeakClass::IncrementValue, weak_object);

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  std::move(callback).Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
}

TEST_F(WeakCallbackTest, InvalidatedWeakOnceCallback) {
  auto callback =
      base::BindOnce(&WeakClass::IncrementValue, weak_object_.GetWeakPtr());

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  weak_object_.InvalidateWeakPtrs();
  std::move(callback).Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
}

TEST_F(WeakCallbackTest, ValidWeakOnceCallback) {
  auto callback =
      base::BindOnce(&WeakClass::IncrementValue, weak_object_.GetWeakPtr());

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  std::move(callback).Run();
  EXPECT_EQ(weak_object_.GetValue(), 1u);
}

TEST_F(WeakCallbackTest, ValidWeakOnceCallbackWithArg) {
  const auto kIncrementByValue = 5u;
  auto callback = base::BindOnce(&WeakClass::IncrementBy,
                                 weak_object_.GetWeakPtr(), kIncrementByValue);

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  std::move(callback).Run();
  EXPECT_EQ(weak_object_.GetValue(), kIncrementByValue);
}

TEST_F(WeakCallbackTest, EmptyWeakRepeatingCallback) {
  base::WeakPtr<WeakClass> weak_object{nullptr};
  auto callback = base::BindRepeating(&WeakClass::IncrementValue, weak_object);

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
}

TEST_F(WeakCallbackTest, InvalidatedWeakRepeatingCallback) {
  auto callback = base::BindRepeating(&WeakClass::IncrementValue,
                                      weak_object_.GetWeakPtr());

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  weak_object_.InvalidateWeakPtrs();
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 0u);
}

TEST_F(WeakCallbackTest, ValidThenInvalidatedWeakRepeatingCallback) {
  auto callback = base::BindRepeating(&WeakClass::IncrementValue,
                                      weak_object_.GetWeakPtr());

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 1u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 2u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u);

  weak_object_.InvalidateWeakPtrs();

  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u);
}

TEST_F(WeakCallbackTest, ValidThenInvalidatedWeakRepeatingCallbackWithArg) {
  const auto kIncrementByValue = 5u;
  auto callback = base::BindRepeating(
      &WeakClass::IncrementBy, weak_object_.GetWeakPtr(), kIncrementByValue);

  EXPECT_EQ(weak_object_.GetValue(), 0u);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 1u * kIncrementByValue);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 2u * kIncrementByValue);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u * kIncrementByValue);

  weak_object_.InvalidateWeakPtrs();

  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u * kIncrementByValue);
  callback.Run();
  EXPECT_EQ(weak_object_.GetValue(), 3u * kIncrementByValue);
}

class ThreadedWeakCallbackTest : public WeakCallbackTest {
 public:
  void SetUp() override { thread_.Start(); }
  void TearDown() override {
    thread_.FlushForTesting();
    thread_.Join();
  }

  std::shared_ptr<base::SequencedTaskRunner> TaskRunner() {
    return thread_.TaskRunner();
  }

  void VerifyExpectation(size_t expected_value) {
    EXPECT_TRUE(TaskRunner()->RunsTasksInCurrentSequence());
    EXPECT_EQ(weak_object_.GetValue(), expected_value);
  }

 protected:
  base::Thread thread_;
};

TEST_F(ThreadedWeakCallbackTest, UseWeakPtrOnAnotherSequence) {
  auto weak_ptr = weak_object_.GetWeakPtr();
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::VerifyExpectation, weak_ptr, 0u));

  // Increment twice
  TaskRunner()->PostTask(FROM_HERE,
                         base::BindOnce(&WeakClass::IncrementValue, weak_ptr));
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::VerifyExpectation, weak_ptr, 1u));
  TaskRunner()->PostTask(FROM_HERE,
                         base::BindOnce(&WeakClass::IncrementValue, weak_ptr));
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::VerifyExpectation, weak_ptr, 2u));

  // Invalidate
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::InvalidateWeakPtrs, weak_ptr));

  // Increment twice with no effect
  TaskRunner()->PostTask(FROM_HERE,
                         base::BindOnce(&WeakClass::IncrementValue, weak_ptr));
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::VerifyExpectation, weak_ptr, 2u));
  TaskRunner()->PostTask(FROM_HERE,
                         base::BindOnce(&WeakClass::IncrementValue, weak_ptr));
  TaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&WeakClass::VerifyExpectation, weak_ptr, 2u));
}

}  // namespace
