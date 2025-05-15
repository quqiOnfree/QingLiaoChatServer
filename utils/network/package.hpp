#ifndef PACKAGE_H
#define PACKAGE_H

#include <string>
#include <string_view>
#include <memory_resource>
#include <system_error>
#include <cstring>
#include <type_traits>

#include "networkEndianness.hpp"
#include "qls_error.h"

namespace qls
{
    
/**
 * @brief A class to handle data packages.
 */
template<class T>
    requires std::is_integral_v<T>
class Package final
{
public:
    Package():
        m_buffer(&local_sync_package_pool)
    {}
    ~Package() noexcept = default;

    Package(const Package&) = delete;
    Package(Package&&) = delete;

    Package& operator=(const Package&) = delete;
    Package& operator=(Package&&) = delete;

    /**
     * @brief Writes data into the class.
     * @param data The binary data to write.
     */
    void write(std::string_view data)
    {
        m_buffer += data;
    }

    /**
     * @brief Checks if data can be read from the package.
     * @return true if data can be read, false otherwise.
     */
    [[nodiscard]] bool canRead() const
    {
        if (m_buffer.size() < sizeof(T))
            return false;

        T length = 0;
        std::memcpy(&length, m_buffer.c_str(), sizeof(T));
        length = qls::swapNetworkEndianness(length);
        return length <= m_buffer.size();
    }

    /**
     * @brief Gets the length of the first message in the package.
     * @return The length of the first message.
     */
    [[nodiscard]] std::size_t firstMsgLength() const
    {
        if (m_buffer.size() < sizeof(T))
            return 0;

        T length = 0;
        std::memcpy(&length, m_buffer.c_str(), sizeof(T));
        length = qls::swapNetworkEndianness(length);
        return std::size_t(length);
    }

    /**
     * @brief Reads a data package.
     * @return The data package.
     */
    [[nodiscard]] std::pmr::string read()
    {
        if (!canRead())
            throw std::system_error(qls_errc::incomplete_package);
        else if (!firstMsgLength())
            throw std::system_error(qls_errc::empty_length);

        std::pmr::string result = { m_buffer.substr(0, firstMsgLength()),
                                    &local_sync_package_pool };
        m_buffer.erase(0, firstMsgLength());

        return result;
    }

    /**
     * @brief Reads the buffer data in the package.
     * @return The buffer as a string.
     */
    [[nodiscard]] std::string_view readBuffer() const
    {
        return m_buffer;
    }

    /**
     * @brief Sets the buffer with the given data.
     * @param buffer The data to set in the buffer.
     */
    void setBuffer(std::string_view buffer)
    {
        m_buffer = buffer;
    }

private:
    std::pmr::string m_buffer;
    static inline std::pmr::synchronized_pool_resource local_sync_package_pool = {};
};

} // namespace qls

#endif // !PACKAGE_H
