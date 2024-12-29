#ifndef DATA_PACKAGE_H
#define DATA_PACKAGE_H

#include <string>
#include <memory>
#include <string_view>

namespace qls
{

/**
 * @class DataPackage
 * @brief Represents a data package with metadata and binary data.
 */
class DataPackage final
{
public:
    enum DataPackageType : int
    {
        Unknown = 0,
        Text = 1,
        Binary = 2,
        FileStream = 3,
        HeartBeat = 4
    };

private:
#pragma pack(1)
    int                 length = 0;                         ///< Length of the data package.
public:
    long long           requestID = 0;                      ///< Request ID associated with the data package.
    DataPackageType     type = DataPackageType::Unknown;    ///< Type identifier of the data package.
    int                 sequence = -1;                      ///< Sequence number of the data package.
private:
    unsigned long long  verifyCode = 0;                     ///< Verification code for the data package.
    char                data[2]{ 0 };                       ///< Data buffer of the data package.
#pragma pack()

public:
    DataPackage() = delete;
    ~DataPackage() = default;
    DataPackage(const DataPackage&) = delete;
    DataPackage(DataPackage&&) = delete;

    DataPackage& operator=(const DataPackage&) = delete;
    DataPackage& operator=(DataPackage&&) = delete;

    /**
     * @brief Creates a data package from the given data.
     * @param data Original data to be stored in the data package.
     * @return Shared pointer to the created data package.
     */
    static std::shared_ptr<DataPackage> makePackage(std::string_view data);

    /**
     * @brief Loads a data package from binary data.
     * @param data Binary data representing a data package.
     * @return Shared pointer to the loaded data package.
     */
    static std::shared_ptr<DataPackage> stringToPackage(std::string_view data);

    /**
     * @brief Converts this data package to a binary string.
     * @return Binary data representing this data package.
     */
    std::string packageToString() noexcept;

    /**
     * @brief Gets the size of this data package.
     * @return Size of this data package.
     */
    size_t getPackageSize() noexcept;

    /**
     * @brief Gets the size of the original data in this data package.
     * @return Size of the original data in this data package.
     */
    size_t getDataSize() noexcept;

    /**
     * @brief Gets the original data in this data package.
     * @return Original data in this data package.
     */
    std::string getData();
};

} // namespace qls

#endif // !DATA_PACKAGE_H
