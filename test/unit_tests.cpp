#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../util/handle.h"
#include "../socket/Socket.h"

class MockSocket
  : public pw_socket::Socket
{
public:
  MockSocket()
    : pw_socket::Socket(39) {}

  ~MockSocket() override = default;
  
  bool create() override {
    return false;
  }

  bool connect() override {
    return false;
  }

  bool send(const void* buffer, std::size_t bytes, std::size_t* bytesWrote = nullptr) override {
    return false;
  }

  bool recv(void* buffer, std::size_t bytes, std::size_t* bytesRcvd = nullptr) override {
    return false;
  }

  // Prevent the base class from calling close on an invalid handle
  MOCK_METHOD(bool, close, ());
};

class MockHandle
  : public pw_util::Handle
{
  public:
  MockHandle(int handle) : Handle(handle) {}
  ~MockHandle() = default;

  // Prevent the base class from calling close on an invalid handle
  MOCK_METHOD(int, close, (int));
};

TEST(handle, functional)
{
  MockHandle h1(3);
  EXPECT_EQ(h1.value(), 3);
  EXPECT_EQ((int)h1, 3);
}

TEST(handle, cast_int)
{
  MockHandle h1(3);
  EXPECT_EQ((int)h1, 3);
}

TEST(handle, equality)
{
  MockHandle h1(3);
  MockHandle h2(3);
  EXPECT_TRUE(h1 == h2);

  MockHandle h3(4);
  EXPECT_TRUE(h1 != h3);
}

TEST(handle, close)
{
  pw_util::Handle h1(4);
  EXPECT_TRUE(h1.close());
  EXPECT_TRUE(h1.value() == pw_util::Handle::InvalidHandleValue);
}

TEST(socket, cast_int)
{
  MockSocket s;
  EXPECT_TRUE((int)s == 39);
}

TEST(socket, equality)
{
  MockSocket s1;
  MockSocket s2;
  EXPECT_TRUE(s1 == s2);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}