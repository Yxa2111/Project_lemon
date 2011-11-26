/***************************************************************************
    This file is part of Project Lemon
    Copyright (C) 2011 Zhipeng Jia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***************************************************************************/

#include <cstring>
#include <cstdio>
#include "judgingthread.h"
#include "settings.h"
#include "task.h"

JudgingThread::JudgingThread(QObject *parent) :
    QThread(parent)
{
    moveToThread(this);
    checkRejudgeMode = false;
    needRejudge = false;
    stopJudging = false;
    timeUsed = -1;
    memoryUsed = -1;
}

void JudgingThread::setCheckRejudgeMode(bool check)
{
    checkRejudgeMode = check;
}

void JudgingThread::setExtraTimeRatio(double ratio)
{
    extraTimeRatio = ratio;
}

void JudgingThread::setEnvironment(const QProcessEnvironment &env)
{
    environment = env;
}

void JudgingThread::setWorkingDirectory(const QString &directory)
{
    workingDirectory = directory;
}

void JudgingThread::setSpecialJudgeTimeLimit(int limit)
{
    specialJudgeTimeLimit = limit;
}

void JudgingThread::setExecutableFile(const QString &fileName)
{
    executableFile = fileName;
}

void JudgingThread::setArguments(const QString &argumentsList)
{
    arguments = argumentsList;
}

void JudgingThread::setAnswerFile(const QString &fileName)
{
    answerFile = fileName;
}

void JudgingThread::setInputFile(const QString &fileName)
{
    inputFile = fileName;
}

void JudgingThread::setOutputFile(const QString &fileName)
{
    outputFile = fileName;
}

void JudgingThread::setTask(Task *_task)
{
    task = _task;
}

void JudgingThread::setFullScore(int score)
{
    fullScore = score;
}

void JudgingThread::setTimeLimit(int limit)
{
    timeLimit = limit;
}

void JudgingThread::setMemoryLimit(int limit)
{
    memoryLimit = limit;
}

int JudgingThread::getTimeUsed() const
{
    return timeUsed;
}

int JudgingThread::getMemoryUsed() const
{
    return memoryUsed;
}

int JudgingThread::getScore() const
{
    return score;
}

ResultState JudgingThread::getResult() const
{
    return result;
}

const QString& JudgingThread::getMessage() const
{
    return message;
}

bool JudgingThread::getNeedRejudge() const
{
    return needRejudge;
}

void JudgingThread::stopJudgingSlot()
{
    stopJudging = true;
}

void JudgingThread::compareLineByLine(const QString &contestantOutput)
{
    FILE *contestantOutputFile = fopen(contestantOutput.toLocal8Bit().data(), "r");
    if (contestantOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open contestant\'s output file");
        return;
    }
    FILE *standardOutputFile = fopen(outputFile.toLocal8Bit().data(), "r");
    if (standardOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open standard output file");
        fclose(contestantOutputFile);
        return;
    }
    
    char str1[20], str2[20], ch;
    bool chk1 = false, chk2 = false;
    bool chkEof1 = false, chkEof2 = false;
    int len1, len2;
    while (true) {
        len1 = 0;
        while (len1 < 10) {
            ch = fgetc(contestantOutputFile);
            if (ch == EOF) break;
            if (! chk1 && ch == '\n') break;
            if (chk1 && ch == '\n') {
                chk1 = false;
                continue;
            }
            if (ch == '\r') {
                chk1 = true;
                break;
            }
            if (chk1) chk1 = false;
            str1[len1 ++] = ch;
        }
        str1[len1 ++] = '\0';
        if (ch == EOF) chkEof1 = true; else chkEof1 = false;
        
        len2 = 0;
        while (len2 < 10) {
            ch = fgetc(standardOutputFile);
            if (ch == EOF) break;
            if (! chk2 && ch == '\n') break;
            if (chk2 && ch == '\n') {
                chk2 = false;
                continue;
            }
            if (ch == '\r') {
                chk2 = true;
                break;
            }
            if (chk2) chk2 = false;
            str2[len2 ++] = ch;
        }
        str2[len2 ++] = '\0';
        if (ch == EOF) chkEof2 = true; else chkEof2 = false;
        
        if (chkEof1 && ! chkEof2) {
            score = 0;
            result = WrongAnswer;
            message = tr("Shorter than standard output");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        
        if (! chkEof1 && chkEof2) {
            score = 0;
            result = WrongAnswer;
            message = tr("Longer than standard output");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        
        if (len1 != len2 || strcmp(str1, str2) != 0) {
            score = 0;
            result = WrongAnswer;
            message = tr("Read %1 but expect %2").arg(str1).arg(str2);
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (chkEof1 && chkEof2) break;
        QCoreApplication::processEvents();
        if (stopJudging) {
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
    }
    
    score = fullScore;
    result = CorrectAnswer;
    fclose(contestantOutputFile);
    fclose(standardOutputFile);
}

void JudgingThread::compareIgnoreSpaces(const QString &contestantOutput)
{
    FILE *contestantOutputFile = fopen(contestantOutput.toLocal8Bit().data(), "r");
    if (contestantOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open contestant\'s output file");
        return;
    }
    FILE *standardOutputFile = fopen(outputFile.toLocal8Bit().data(), "r");
    if (standardOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open standard output file");
        fclose(contestantOutputFile);
        return;
    }
    
    char ch1 = '\n', ch2 = '\n';
    char str1[20], str2[20];
    int flag1, flag2;
    while (true) {
        if (ch1 == '\n' || ch1 == '\r' || ch1 == EOF) {
            if (ch1 == '\r') {
                ch1 = fgetc(contestantOutputFile);
                if (ch1 == '\n') ch1 = fgetc(contestantOutputFile);
            } else
                ch1 = fgetc(contestantOutputFile);
            while (ch1 == ' ' || ch1 == '\t')
                ch1 = fgetc(contestantOutputFile);
            flag1 = 2;
        } else
            if (ch1 == ' ' || ch1 == '\t') {
                while (ch1 == ' ' || ch1 == '\t')
                    ch1 = fgetc(contestantOutputFile);
                if (ch1 == '\n' || ch1 == '\r' || ch1 == EOF) {
                    if (ch1 == '\r') {
                        ch1 = fgetc(contestantOutputFile);
                        if (ch1 == '\n') ch1 = fgetc(contestantOutputFile);
                    } else
                        ch1 = fgetc(contestantOutputFile);
                    while (ch1 == ' ' || ch1 == '\t')
                        ch1 = fgetc(contestantOutputFile);
                    flag1 = 2;
                } else
                    flag1 = 1;
            } else
                flag1 = 0;
        
        if (ch2 == '\n' || ch2 == '\r' || ch2 == EOF) {
            if (ch2 == '\r') {
                ch2 = fgetc(standardOutputFile);
                if (ch2 == '\n') ch2 = fgetc(standardOutputFile);
            } else
                ch2 = fgetc(standardOutputFile);
            while (ch2 == ' ' || ch2 == '\t')
                ch2 = fgetc(standardOutputFile);
            flag2 = 2;
        } else
            if (ch2 == ' ' || ch2 == '\t') {
                while (ch2 == ' ' || ch2 == '\t')
                    ch2 = fgetc(standardOutputFile);
                if (ch2 == '\n' || ch2 == '\r' || ch2 == EOF) {
                    if (ch2 == '\r') {
                        ch2 = fgetc(standardOutputFile);
                        if (ch2 == '\n') ch2 = fgetc(standardOutputFile);
                    } else
                        ch2 = fgetc(standardOutputFile);
                    while (ch2 == ' ' || ch2 == '\t')
                        ch2 = fgetc(standardOutputFile);
                    flag2 = 2;
                } else
                    flag2 = 1;
            } else
                flag2 = 0;
        
        if (flag1 != flag2) {
            score = 0;
            result = WrongAnswer;
            message = tr("Presentation error");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        
        int len1 = 0;
        while (len1 < 10) {
            if (ch1 != ' ' && ch1 != '\t' && ch1 != '\n' && ch1 != '\r' && ch1 != EOF)
                str1[len1 ++] = ch1;
            else
                break;
            ch1 = fgetc(contestantOutputFile);
        }
        str1[len1] = '\0';
        
        int len2 = 0;
        while (len2 < 10) {
            if (ch2 != ' ' && ch2 != '\t' && ch2 != '\n' && ch2 != '\r' && ch2 != EOF)
                str2[len2 ++] = ch2;
            else
                break;
            ch2 = fgetc(standardOutputFile);
        }
        str2[len2] = '\0';
        
        if (len1 != len2 || strcmp(str1, str2) != 0) {
            score = 0;
            result = WrongAnswer;
            message = tr("Read %1 but expect %2").arg(str1).arg(str2);
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (ch1 == EOF && ch2 == EOF) break;
        
        QCoreApplication::processEvents();
        if (stopJudging) {
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
    }
    
    score = fullScore;
    result = CorrectAnswer;
    fclose(contestantOutputFile);
    fclose(standardOutputFile);
}

void JudgingThread::compareRealNumbers(const QString &contestantOutput)
{
    FILE *contestantOutputFile = fopen(contestantOutput.toLocal8Bit().data(), "r");
    if (contestantOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open contestant\'s output file");
        return;
    }
    FILE *standardOutputFile = fopen(outputFile.toLocal8Bit().data(), "r");
    if (standardOutputFile == NULL) {
        score = 0;
        result = FileError;
        message = tr("Cannot open standard output file");
        fclose(contestantOutputFile);
        return;
    }
    
    double eps = 1;
    for (int i = 0; i < task->getRealPrecision(); i ++)
        eps *= 0.1;
    
    double a, b;
    while (true) {
        int cnt1 = fscanf(contestantOutputFile, "%lf", &a);
        int cnt2 = fscanf(standardOutputFile, "%lf", &b);
        if (cnt1 == 0) {
            score = 0;
            result = WrongAnswer;
            message = tr("Invalid characters found");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (cnt2 == 0) {
            score = 0;
            result = FileError;
            message = tr("Invalid characters in standard output file");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (cnt1 == EOF && cnt2 == EOF) break;
        if (cnt1 == EOF && cnt2 == 1) {
            score = 0;
            result = WrongAnswer;
            message = tr("Shorter than standard output");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (cnt1 == 1 && cnt2 == EOF) {
            score = 0;
            result = WrongAnswer;
            message = tr("Longer than standard output");
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        if (fabs(a - b) > eps) {
            score = 0;
            result = WrongAnswer;
            message = tr("Read %1 but expect %2").arg(a, 0, 'g', 18).arg(b, 0, 'g', 18);
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
        QCoreApplication::processEvents();
        if (stopJudging) {
            fclose(contestantOutputFile);
            fclose(standardOutputFile);
            return;
        }
    }
    
    score = fullScore;
    result = CorrectAnswer;
    fclose(contestantOutputFile);
    fclose(standardOutputFile);
}

void JudgingThread::specialJudge(const QString &fileName)
{
    if (! QFileInfo(inputFile).exists()) {
        score = 0;
        result = FileError;
        message = tr("Cannot find standard input file");
        return;
    }
    
    if (! QFileInfo(fileName).exists()) {
        score = 0;
        result = FileError;
        message = tr("Cannot find contestant\'s output file");
        return;
    }
    
    if (! QFileInfo(outputFile).exists()) {
        score = 0;
        result = FileError;
        message = tr("Cannot find standard output file");
        return;
    }
    
    QProcess *judge = new QProcess(this);
    QStringList arguments;
    arguments << inputFile << fileName << outputFile << QString("%1").arg(fullScore);
    arguments << workingDirectory + "_score";
    arguments << workingDirectory + "_message";
    judge->start(Settings::dataPath() + task->getSpecialJudge(), arguments);
    if (! judge->waitForStarted(-1)) {
        score = 0;
        result = InvalidSpecialJudge;
        delete judge;
        return;
    }
    
    QElapsedTimer timer;
    timer.start();
    bool flag = false;
    while (timer.elapsed() < specialJudgeTimeLimit) {
        if (judge->state() != QProcess::Running) {
            flag = true;
            break;
        }
        QCoreApplication::processEvents();
        if (stopJudging) {
            judge->kill();
            delete judge;
            return;
        }
        msleep(10);
    }
    if (! flag) {
        judge->kill();
        score = 0;
        result = SpecialJudgeTimeLimitExceeded;
        delete judge;
        return;
    } else
        if (judge->exitCode() != 0) {
            score = 0;
            result = SpecialJudgeRunTimeError;
            delete judge;
            return;
        }
    delete judge;
    
    QFile scoreFile(workingDirectory + "_score");
    if (! scoreFile.open(QFile::ReadOnly)) {
        score = 0;
        result = InvalidSpecialJudge;
        return;
    }
    
    QTextStream scoreStream(&scoreFile);
    scoreStream >> score;
    if (scoreStream.status() == QTextStream::ReadCorruptData) {
        score = 0;
        result = InvalidSpecialJudge;
        return;
    }
    scoreFile.close();
    
    if (score < 0) {
        score = 0;
        result = InvalidSpecialJudge;
        return;
    }
    
    QFile messageFile(workingDirectory + "_message");
    if (messageFile.open(QFile::ReadOnly)) {
        QTextStream messageStream(&messageFile);
        message = messageStream.readAll();
        messageFile.close();
    }
    
    if (score == 0) result = WrongAnswer;
    if (0 < score && score < fullScore) result = PartlyCorrect;
    if (score >= fullScore) result = CorrectAnswer;
    
    scoreFile.remove();
    messageFile.remove();
}

void JudgingThread::runProgram()
{
    result = CorrectAnswer;
    int extraTime = qCeil(qMax(2000, timeLimit * 2) * extraTimeRatio);
    
#ifdef Q_OS_WIN32
    QFile::copy(":/runner/runner_win32.exe", workingDirectory + "runner.exe");
    QProcess::execute(QString("attrib -R \"") + workingDirectory + "runner.exe" + "\"");
#endif
    
#ifdef Q_OS_LINUX
    QFile::copy(":/runner/runner_unix", workingDirectory + "runner");
    QProcess::execute(QString("chmod +wx \"") + workingDirectory + "runner" + "\"");
#endif
    
    QProcess *runner = new QProcess(this);
    QStringList argumentsList;
    argumentsList << executableFile << QString("\"%1\" %2").arg(executableFile, arguments);
    if (task->getStandardInputCheck())
        argumentsList << QFileInfo(inputFile).absoluteFilePath().replace('/', QDir::separator());
    else
        argumentsList << "";
    if (task->getStandardOutputCheck())
        argumentsList << "_tmpout";
    else
        argumentsList << "";
    argumentsList << "_tmperr";
    argumentsList << QString("%1").arg(timeLimit + extraTime);
    argumentsList << QString("%1").arg(memoryLimit);
    runner->setProcessEnvironment(environment);
    runner->setWorkingDirectory(workingDirectory);
#ifdef Q_OS_WIN32
    runner->start(workingDirectory + "runner.exe", argumentsList);
#endif
#ifdef Q_OS_LINUX
    runner->start(workingDirectory + "runner", argumentsList);
#endif
    if (! runner->waitForStarted(-1)) {
        delete runner;
        score = 0;
        result = CannotStartProgram;
        return;
    }
    
#ifdef Q_OS_WIN32
    while (true) {
        if (runner->state() != QProcess::Running) break;
        QCoreApplication::processEvents();
        if (stopJudging) {
            runner->waitForFinished(-1);
            delete runner;
            return;
        }
        msleep(10);
    }
#endif
    
#ifdef Q_OS_LINUX
    bool flag = false;
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeLimit + extraTime) {
        if (runner->state() != QProcess::Running) {
            flag = true;
            break;
        }
        QCoreApplication::processEvents();
        if (stopJudging) {
            runner->terminate();
            delete runner;
            return;
        }
        msleep(10);
    }
    
    if (! flag) {
        runner->terminate();
        delete runner;
        score = 0;
        result = TimeLimitExceeded;
        timeUsed = memoryLimit = -1;
        return;
    }
#endif
    
    int code = runner->exitCode();
    
    if (code == 1) {
        delete runner;
        score = 0;
        result = CannotStartProgram;
        timeUsed = memoryUsed = -1;
        return;
    }
    
    if (code == 2) {
        delete runner;
        score = 0;
        result = RunTimeError;
        QFile file(workingDirectory + "_tmperr");
        if (file.open(QFile::ReadOnly)) {
            QTextStream stream(&file);
            message = stream.readAll();
            file.close();
        }
        timeUsed = memoryUsed = -1;
        return;
    }
    
    QString out = QString::fromLocal8Bit(runner->readAllStandardOutput().data());
    QTextStream stream(&out, QIODevice::ReadOnly);
    stream >> timeUsed >> memoryUsed;
    
    if (memoryUsed <= 0) memoryLimit = -1;
    
    if (code == 3) {
        delete runner;
        score = 0;
        result = TimeLimitExceeded;
        timeUsed = -1;
        return;
    }
    
    if (code == 4) {
        delete runner;
        score = 0;
        result = MemoryLimitExceeded;
        memoryUsed = -1;
        return;
    }
    
    delete runner;
}

void JudgingThread::judgeOutput()
{
    if (task->getComparisonMode() == Task::LineByLineMode) {
        if (task->getStandardOutputCheck())
            compareLineByLine(workingDirectory + "_tmpout");
        else
            compareLineByLine(workingDirectory + task->getOutputFileName());
    }
    
    if (task->getComparisonMode() == Task::IgnoreSpacesMode) {
        if (task->getStandardOutputCheck())
            compareIgnoreSpaces(workingDirectory + "_tmpout");
        else
            compareIgnoreSpaces(workingDirectory + task->getOutputFileName());
    }
    
    if (task->getComparisonMode() == Task::RealNumberMode) {
        if (task->getStandardOutputCheck())
            compareRealNumbers(workingDirectory + "_tmpout");
        else
            compareRealNumbers(workingDirectory + task->getOutputFileName());
    }
    
    if (task->getComparisonMode() == Task::SpecialJudgeMode) {
        if (task->getStandardOutputCheck())
            specialJudge(workingDirectory + "_tmpout");
        else
            specialJudge(workingDirectory + task->getOutputFileName());
    }
}

void JudgingThread::judgeTraditionalTask()
{
    if (! QFileInfo(inputFile).exists()) {
        score = 0;
        result = FileError;
        message = tr("Cannot find standard input file");
        return;
    }
    if (! task->getStandardInputCheck())
        if (! QFile::copy(inputFile, workingDirectory + task->getInputFileName())) {
            score = 0;
            result = FileError;
            message = tr("Cannot copy standard input file");
            return;
        }
    
    runProgram();
    if (stopJudging) return;
    
    if (result != CorrectAnswer) {
        if (! task->getStandardInputCheck())
            QFile::remove(workingDirectory + task->getInputFileName());
        if (! task->getStandardOutputCheck())
            QFile::remove(workingDirectory + task->getOutputFileName());
        else
            QFile::remove(workingDirectory + "_tmpout");
        return;
    }
    
    judgeOutput();
    if (stopJudging) return;
    
    if (timeUsed > timeLimit)
        if (checkRejudgeMode && score > 0 && (timeUsed <= timeLimit * (1 + extraTimeRatio)
                                              || timeUsed <= timeLimit + 1000 * extraTimeRatio)) {
            int minTimeUsed = timeUsed, curMemoryUsed = memoryUsed;
            bool flag = true;
            for (int i = 0; i < 10; i ++) {
                runProgram();
                if (stopJudging) return;
                if (result != CorrectAnswer) {
                    flag = false;
                    break;
                }
                if (timeUsed < minTimeUsed) {
                    minTimeUsed = timeUsed;
                    curMemoryUsed = memoryUsed;
                    judgeOutput();
                    if (stopJudging) return;
                    if (timeUsed <= timeLimit) break;
                }
            }
            timeUsed = minTimeUsed;
            memoryUsed = curMemoryUsed;
            if (! flag || timeUsed > timeLimit) {
                score = 0;
                result = TimeLimitExceeded;
                message = "";
            }
        } else {
            if (! checkRejudgeMode && score > 0 && (timeUsed <= timeLimit * (1 + extraTimeRatio)
                                                    || timeUsed <= timeLimit + 1000 * extraTimeRatio)) {
                needRejudge = true;
            }
            score = 0;
            result = TimeLimitExceeded;
            message = "";
        }
    
    if (! task->getStandardInputCheck())
        QFile::remove(workingDirectory + task->getInputFileName());
    if (! task->getStandardOutputCheck())
        QFile::remove(workingDirectory + task->getOutputFileName());
    else
        QFile::remove(workingDirectory + "_tmpout");
}

void JudgingThread::judgeAnswersOnlyTask()
{
    if (task->getComparisonMode() == Task::LineByLineMode)
        compareLineByLine(answerFile);
    if (task->getComparisonMode() == Task::IgnoreSpacesMode)
        compareIgnoreSpaces(answerFile);
    if (task->getComparisonMode() == Task::RealNumberMode)
        compareRealNumbers(answerFile);
    if (task->getComparisonMode() == Task::SpecialJudgeMode)
        specialJudge(answerFile);
}

void JudgingThread::run()
{
    if (task->getTaskType() == Task::Traditional)
        judgeTraditionalTask();
    if (task->getTaskType() == Task::AnswersOnly)
        judgeAnswersOnlyTask();
}
