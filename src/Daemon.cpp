//
// Created by Stephane Bourque on 2021-05-07.
//

#include <aws/core/Aws.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/AccessControlPolicy.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/s3/model/GetBucketAclRequest.h>

#include "Daemon.h"
#include "StorageService.h"
#include "ManifestCreator.h"
#include "NewConnectionHandler.h"
#include "LatestFirmwareCache.h"
#include "DeviceCache.h"
#include "FirmwareCache.h"
#include "AutoUpdater.h"
#include "NewCommandHandler.h"

namespace OpenWifi {
    class Daemon *Daemon::instance_ = nullptr;

    class Daemon *Daemon::instance() {
        if (instance_ == nullptr) {
            instance_ = new Daemon(vDAEMON_PROPERTIES_FILENAME,
                                   vDAEMON_ROOT_ENV_VAR,
                                   vDAEMON_CONFIG_ENV_VAR,
                                   vDAEMON_APP_NAME,
                                   vDAEMON_BUS_TIMER,
                                   SubSystemVec{
                                            StorageService(),
                                            FirmwareCache(),
                                            LatestFirmwareCache(),
                                            DeviceCache(),
                                            NewConnectionHandler(),
                                            ManifestCreator(),
                                            AutoUpdater(),
                                            NewCommandHandler()
                                   });
        }
        return instance_;
    }

    void Daemon::PostInitialization([[maybe_unused]] Poco::Util::Application &self) {
    }
}

Aws::Utils::Logging::LogLevel AwsLogParseLevel(const std::string& level)
{
	if (Poco::icompare(level, "Off") == 0) {
		return Aws::Utils::Logging::LogLevel::Off;
	} else if (Poco::icompare(level, "Fatal") == 0) {
		return Aws::Utils::Logging::LogLevel::Fatal;
	} else if (Poco::icompare(level, "Error") == 0) {
		return Aws::Utils::Logging::LogLevel::Error;
	} else if (Poco::icompare(level, "Warn") == 0) {
		return Aws::Utils::Logging::LogLevel::Warn;
	} else if (Poco::icompare(level, "Info") == 0) {
		return Aws::Utils::Logging::LogLevel::Info;
	} else if (Poco::icompare(level, "Debug") == 0) {
		return Aws::Utils::Logging::LogLevel::Debug;
	} else if (Poco::icompare(level, "Trace") == 0) {
		return Aws::Utils::Logging::LogLevel::Trace;
	} else {
		int numLevel;
		if (Poco::NumberParser::tryParse(level, numLevel))
		{
			if (numLevel >= 0 && numLevel <= 6) {
				return (Aws::Utils::Logging::LogLevel)numLevel;
			} else {
				return Aws::Utils::Logging::LogLevel::Off;
            }
		} else {
			return Aws::Utils::Logging::LogLevel::Off;
        }
	}
}

int main(int argc, char **argv) {
    SSL_library_init();
    Aws::SDKOptions AwsOptions;
    AwsOptions.memoryManagementOptions.memoryManager = nullptr;
    AwsOptions.cryptoOptions.initAndCleanupOpenSSL = false;
    AwsOptions.httpOptions.initAndCleanupCurl = true;

    Aws::String AwsLogLevel = Poco::Environment::get("S3_SDK_LOG_LEVLE", "Off");
    AwsOptions.loggingOptions.logLevel = AwsLogParseLevel(AwsLogLevel);
    std::string LogPrefix = Poco::Environment::get("S3_SDK_LOG_PREFIX", "aws_sdk_");
    AwsOptions.loggingOptions.defaultLogPrefix = LogPrefix.c_str();

    std::cout << "AwsOptions logLevel is : " << (int)AwsOptions.loggingOptions.logLevel << std::endl;
    std::cout << "AwsOptions defaultLogPrefix is : " << LogPrefix.c_str() << std::endl;

    Aws::InitAPI(AwsOptions);

    int ExitCode=0;
    {
        auto App = OpenWifi::Daemon::instance();
        ExitCode = App->run(argc, argv);
    }

    ShutdownAPI(AwsOptions);
    return ExitCode;
}
