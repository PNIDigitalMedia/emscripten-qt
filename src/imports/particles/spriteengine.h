#ifndef SPRITEENGINE_H
#define SPRITEENGINE_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QTime>
#include <QList>
#include <QDeclarativeListProperty>
#include <QImage>
#include <QPair>

class SpriteState;

class SpriteEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDeclarativeListProperty<SpriteState> sprites READ sprites)
    Q_PROPERTY(QString globalGoal READ globalGoal WRITE setGlobalGoal NOTIFY globalGoalChanged)
public:
    explicit SpriteEngine(QObject *parent = 0);
    QDeclarativeListProperty<SpriteState> sprites()
    {
        return QDeclarativeListProperty<SpriteState>(this, m_states);
    }
    QString globalGoal() const
    {
        return m_globalGoal;
    }

    int count() const {return m_sprites.count();}
    void setCount(int c);

    int spriteState(int sprite=0) {return m_sprites[sprite];}

    int stateIndex(SpriteState* s){return m_states.indexOf(s);}
    SpriteState* state(int idx){return m_states[idx];}
    int stateCount() {return m_states.count();}
    int maxFrames();

    void setGoal(int state, int sprite=0);
    QImage assembledImage();

    void startSprite(int index=0);

signals:

    void globalGoalChanged(QString arg);

public slots:
    void setGlobalGoal(QString arg)
    {
        if (m_globalGoal != arg) {
            m_globalGoal = arg;
            emit globalGoalChanged(arg);
        }
    }

    uint updateSprites(uint time);

private:
    void addToUpdateList(uint t, int idx);
    int goalSeek(int curState, int spriteIdx, int dist=-1);
    QList<SpriteState*> m_states;
    QVector<int> m_sprites;//int is the index in m_states of the current state
    QVector<int> m_goals;
    QList<QPair<uint, QList<int> > > m_stateUpdates;//### This could be done faster

    QTime m_advanceTime;
    uint m_timeOffset;
    QString m_globalGoal;
    int m_maxFrames;
};

#endif // SPRITEENGINE_H