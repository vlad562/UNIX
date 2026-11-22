#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t was_sighup = 0;

static void sighup_handler(int signal_number)
{
  was_sighup = 1;
}

int main()
{
  const int port = 3000;

  int server_fd = -1;
  int client_fd = -1;

  sigset_t blocked_mask;
  sigset_t original_mask;
  sigset_t wait_mask;

  sigemptyset(&blocked_mask);
  sigaddset(&blocked_mask, SIGHUP);

  if (sigprocmask(SIG_BLOCK, &blocked_mask, &original_mask) == -1)
  {
    perror("sigprocmask");
    return EXIT_FAILURE;
  }

  struct sigaction action{};

  action.sa_handler = sighup_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGHUP, &action, nullptr) == -1)
  {
    perror("sigaction");
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  wait_mask = original_mask;
  sigdelset(&wait_mask, SIGHUP);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd == -1)
  {
    perror("socket");
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  if (server_fd >= FD_SETSIZE)
  {
    std::cerr << "server_fd exceeds FD_SETSIZE\n";
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  int reuse = 1;

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                 &reuse, sizeof(reuse)) == -1)
  {
    perror("setsockopt");
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  struct sockaddr_in address{};

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address,
           sizeof(address)) == -1)
  {
    perror("bind");
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  if (listen(server_fd, 16) == -1)
  {
    perror("listen");
    close(server_fd);
    sigprocmask(SIG_SETMASK, &original_mask, nullptr);
    return EXIT_FAILURE;
  }

  std::cout << "Server started on port "
            << port << std::endl;

  while (true)
  {
    fd_set read_fds;

    FD_ZERO(&read_fds);

    FD_SET(server_fd, &read_fds);

    if (client_fd != -1)
      FD_SET(client_fd, &read_fds);

    int max_fd = server_fd;

    if (client_fd > max_fd)
      max_fd = client_fd;

    int result = pselect(
        max_fd + 1,
        &read_fds,
        nullptr,
        nullptr,
        nullptr,
        &wait_mask);

    if (was_sighup)
    {
      was_sighup = 0;
      std::cout << "Received SIGHUP signal!"
                << std::endl;
    }

    if (result == -1)
    {
      if (errno == EINTR)
        continue;

      perror("pselect");
      break;
    }

    // Проверяем готовность клиента.
    if (client_fd != -1 &&
        FD_ISSET(client_fd, &read_fds))
    {
      char buffer[4096];

      ssize_t received = recv(
          client_fd,
          buffer,
          sizeof(buffer),
          0);

      if (received > 0)
      {
        std::cout << "Received "
                  << received
                  << " bytes from client"
                  << std::endl;
      }
      else if (received == 0)
      {
        std::cout << "Client disconnected"
                  << std::endl;

        close(client_fd);
        client_fd = -1;
      }
      else if (errno == EINTR)
      {
        continue;
      }
      else
      {
        perror("recv");
        close(client_fd);
        client_fd = -1;
      }
    }

    // Проверяем наличие новых подключений.
    if (FD_ISSET(server_fd, &read_fds))
    {
      struct sockaddr_in client_address{};
      socklen_t client_length = sizeof(client_address);

      int new_fd = accept(
          server_fd,
          (struct sockaddr *)&client_address,
          &client_length);

      if (new_fd == -1)
      {
        if (errno == EINTR)
          continue;

        perror("accept");
        continue;
      }

      if (new_fd >= FD_SETSIZE)
      {
        std::cerr << "Descriptor exceeds FD_SETSIZE"
                  << std::endl;
        close(new_fd);
        continue;
      }

      char ip[INET_ADDRSTRLEN];

      inet_ntop(
          AF_INET,
          &client_address.sin_addr,
          ip,
          sizeof(ip));

      std::cout << "New connection from "
                << ip << ":"
                << ntohs(client_address.sin_port)
                << std::endl;

      if (client_fd != -1)
      {
        std::cout << "Connection rejected"
                  << std::endl;

        close(new_fd);
      }
      else
      {
        client_fd = new_fd;

        std::cout << "Client accepted"
                  << std::endl;
      }
    }
  }

  if (client_fd != -1)
    close(client_fd);

  close(server_fd);

  if (sigprocmask(SIG_SETMASK, &original_mask, nullptr) == -1)
  {
    perror("sigprocmask restore");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
