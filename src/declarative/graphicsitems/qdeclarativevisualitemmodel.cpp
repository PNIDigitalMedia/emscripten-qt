/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativevisualitemmodel_p.h"

#include "qdeclarativeitem.h"

#include <qdeclarativecontext.h>
#include <qdeclarativecontext_p.h>
#include <qdeclarativeengine.h>
#include <qdeclarativeexpression.h>
#include <qdeclarativepackage_p.h>
#include <qdeclarativeopenmetaobject_p.h>
#include <qdeclarativelistaccessor_p.h>
#include <qdeclarativeinfo.h>
#include <qdeclarativedata_p.h>
#include <qdeclarativepropertycache_p.h>
#include <qdeclarativeguard_p.h>
#include <qdeclarativeglobal_p.h>

#include <qgraphicsscene.h>
#include <qlistmodelinterface_p.h>
#include <qhash.h>
#include <qlist.h>
#include <qmetaobjectbuilder_p.h>
#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

QHash<QObject*, QDeclarativeVisualItemModelAttached*> QDeclarativeVisualItemModelAttached::attachedProperties;


class QDeclarativeVisualItemModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDeclarativeVisualItemModel)
public:
    QDeclarativeVisualItemModelPrivate() : QObjectPrivate() {}

    static void children_append(QDeclarativeListProperty<QDeclarativeItem> *prop, QDeclarativeItem *item) {
        QDeclarative_setParent_noEvent(item, prop->object);
        static_cast<QDeclarativeVisualItemModelPrivate *>(prop->data)->children.append(Item(item));
        static_cast<QDeclarativeVisualItemModelPrivate *>(prop->data)->itemAppended();
        static_cast<QDeclarativeVisualItemModelPrivate *>(prop->data)->emitChildrenChanged();
    }

    static int children_count(QDeclarativeListProperty<QDeclarativeItem> *prop) {
        return static_cast<QDeclarativeVisualItemModelPrivate *>(prop->data)->children.count();
    }

    static QDeclarativeItem *children_at(QDeclarativeListProperty<QDeclarativeItem> *prop, int index) {
        return static_cast<QDeclarativeVisualItemModelPrivate *>(prop->data)->children.at(index).item;
    }

    void itemAppended() {
        Q_Q(QDeclarativeVisualItemModel);
        QDeclarativeVisualItemModelAttached *attached = QDeclarativeVisualItemModelAttached::properties(children.last().item);
        attached->setIndex(children.count()-1);
        emit q->itemsInserted(children.count()-1, 1);
        emit q->countChanged();
    }

    void emitChildrenChanged() {
        Q_Q(QDeclarativeVisualItemModel);
        emit q->childrenChanged();
    }

    int indexOf(QDeclarativeItem *item) const {
        for (int i = 0; i < children.count(); ++i)
            if (children.at(i).item == item)
                return i;
        return -1;
    }

    class Item {
    public:
        Item(QDeclarativeItem *i) : item(i), ref(0) {}

        void addRef() { ++ref; }
        bool deref() { return --ref == 0; }

        QDeclarativeItem *item;
        int ref;
    };

    QList<Item> children;
};


/*!
    \qmlclass VisualItemModel QDeclarativeVisualItemModel
    \ingroup qml-working-with-data
  \since 4.7
    \brief The VisualItemModel allows items to be provided to a view.

    A VisualItemModel contains the visual items to be used in a view.
    When a VisualItemModel is used in a view, the view does not require
    a delegate since the VisualItemModel already contains the visual
    delegate (items).

    An item can determine its index within the
    model via the \l{VisualItemModel::index}{index} attached property.

    The example below places three colored rectangles in a ListView.
    \code
    import Qt 4.7

    Rectangle {
        VisualItemModel {
            id: itemModel
            Rectangle { height: 30; width: 80; color: "red" }
            Rectangle { height: 30; width: 80; color: "green" }
            Rectangle { height: 30; width: 80; color: "blue" }
        }

        ListView {
            anchors.fill: parent
            model: itemModel
        }
    }
    \endcode

    \image visualitemmodel.png

    \sa {declarative/modelviews/visualitemmodel}{VisualItemModel example}
*/
QDeclarativeVisualItemModel::QDeclarativeVisualItemModel(QObject *parent)
    : QDeclarativeVisualModel(*(new QDeclarativeVisualItemModelPrivate), parent)
{
}

/*!
    \qmlattachedproperty int VisualItemModel::index
    This attached property holds the index of this delegate's item within the model.

    It is attached to each instance of the delegate.
*/

QDeclarativeListProperty<QDeclarativeItem> QDeclarativeVisualItemModel::children()
{
    Q_D(QDeclarativeVisualItemModel);
    return QDeclarativeListProperty<QDeclarativeItem>(this, d, d->children_append,
                                                      d->children_count, d->children_at);
}

/*!
    \qmlproperty int VisualItemModel::count

    The number of items in the model.  This property is readonly.
*/
int QDeclarativeVisualItemModel::count() const
{
    Q_D(const QDeclarativeVisualItemModel);
    return d->children.count();
}

bool QDeclarativeVisualItemModel::isValid() const
{
    return true;
}

QDeclarativeItem *QDeclarativeVisualItemModel::item(int index, bool)
{
    Q_D(QDeclarativeVisualItemModel);
    QDeclarativeVisualItemModelPrivate::Item &item = d->children[index];
    item.addRef();
    return item.item;
}

QDeclarativeVisualModel::ReleaseFlags QDeclarativeVisualItemModel::release(QDeclarativeItem *item)
{
    Q_D(QDeclarativeVisualItemModel);
    int idx = d->indexOf(item);
    if (idx >= 0) {
        if (d->children[idx].deref()) {
            if (item->scene())
                item->scene()->removeItem(item);
            QDeclarative_setParent_noEvent(item, this);
        }
    }
    return 0;
}

bool QDeclarativeVisualItemModel::completePending() const
{
    return false;
}

void QDeclarativeVisualItemModel::completeItem()
{
    // Nothing to do
}

QString QDeclarativeVisualItemModel::stringValue(int index, const QString &name)
{
    Q_D(QDeclarativeVisualItemModel);
    if (index < 0 || index >= d->children.count())
        return QString();
    return QDeclarativeEngine::contextForObject(d->children.at(index).item)->contextProperty(name).toString();
}

QVariant QDeclarativeVisualItemModel::evaluate(int index, const QString &expression, QObject *objectContext)
{
    Q_D(QDeclarativeVisualItemModel);
    if (index < 0 || index >= d->children.count())
        return QVariant();
    QDeclarativeContext *ccontext = qmlContext(this);
    QDeclarativeContext *ctxt = new QDeclarativeContext(ccontext);
    ctxt->setContextObject(d->children.at(index).item);
    QDeclarativeExpression e(ctxt, objectContext, expression);
    QVariant value = e.evaluate();
    delete ctxt;
    return value;
}

int QDeclarativeVisualItemModel::indexOf(QDeclarativeItem *item, QObject *) const
{
    Q_D(const QDeclarativeVisualItemModel);
    return d->indexOf(item);
}

QDeclarativeVisualItemModelAttached *QDeclarativeVisualItemModel::qmlAttachedProperties(QObject *obj)
{
    return QDeclarativeVisualItemModelAttached::properties(obj);
}

//============================================================================

class VDMDelegateDataType : public QDeclarativeOpenMetaObjectType
{
public:
    VDMDelegateDataType(const QMetaObject *base, QDeclarativeEngine *engine) : QDeclarativeOpenMetaObjectType(base, engine) {}

    void propertyCreated(int, QMetaPropertyBuilder &prop) {
        prop.setWritable(false);
    }
};

class QDeclarativeVisualDataModelParts;
class QDeclarativeVisualDataModelData;
class QDeclarativeVisualDataModelPrivate : public QObjectPrivate
{
public:
    QDeclarativeVisualDataModelPrivate(QDeclarativeContext *);

    static QDeclarativeVisualDataModelPrivate *get(QDeclarativeVisualDataModel *m) {
        return static_cast<QDeclarativeVisualDataModelPrivate *>(QObjectPrivate::get(m));
    }

    QDeclarativeGuard<QListModelInterface> m_listModelInterface;
    QDeclarativeGuard<QAbstractItemModel> m_abstractItemModel;
    QDeclarativeGuard<QDeclarativeVisualDataModel> m_visualItemModel;
    QString m_part;

    QDeclarativeComponent *m_delegate;
    QDeclarativeGuard<QDeclarativeContext> m_context;
    QList<int> m_roles;
    QHash<QByteArray,int> m_roleNames;
    void ensureRoles() {
        if (m_roleNames.isEmpty()) {
            if (m_listModelInterface) {
                m_roles = m_listModelInterface->roles();
                for (int ii = 0; ii < m_roles.count(); ++ii)
                    m_roleNames.insert(m_listModelInterface->toString(m_roles.at(ii)).toUtf8(), m_roles.at(ii));
                if (m_roles.count() == 1)
                    m_roleNames.insert("modelData", m_roles.at(0));
            } else if (m_abstractItemModel) {
                for (QHash<int,QByteArray>::const_iterator it = m_abstractItemModel->roleNames().begin();
                        it != m_abstractItemModel->roleNames().end(); ++it) {
                    m_roles.append(it.key());
                    m_roleNames.insert(*it, it.key());
                }
                if (m_roles.count() == 1)
                    m_roleNames.insert("modelData", m_roles.at(0));
                if (m_roles.count())
                    m_roleNames.insert("hasModelChildren", -1);
            } else if (m_listAccessor) {
                m_roleNames.insert("modelData", 0);
                if (m_listAccessor->type() == QDeclarativeListAccessor::Instance) {
                    if (QObject *object = m_listAccessor->at(0).value<QObject*>()) {
                        int count = object->metaObject()->propertyCount();
                        for (int ii = 1; ii < count; ++ii) {
                            const QMetaProperty &prop = object->metaObject()->property(ii);
                            m_roleNames.insert(prop.name(), 0);
                        }
                    }
                }
            }
        }
    }

    QHash<int,int> roleToPropId;
    void createMetaData() {
        if (!m_metaDataCreated) {
            ensureRoles();
            if (m_roleNames.count()) {
                QHash<QByteArray, int>::const_iterator it = m_roleNames.begin();
                while (it != m_roleNames.end()) {
                    int propId = m_delegateDataType->createProperty(it.key()) - m_delegateDataType->propertyOffset();
                    roleToPropId.insert(*it, propId);
                    ++it;
                }
                m_metaDataCreated = true;
            }
        }
    }

    struct ObjectRef {
        ObjectRef(QObject *object=0) : obj(object), ref(1) {}
        QObject *obj;
        int ref;
    };
    class Cache : public QHash<int, ObjectRef> {
    public:
        QObject *getItem(int index) {
            QObject *item = 0;
            QHash<int,ObjectRef>::iterator it = find(index);
            if (it != end()) {
                (*it).ref++;
                item = (*it).obj;
            }
            return item;
        }
        QObject *item(int index) {
            QObject *item = 0;
            QHash<int, ObjectRef>::const_iterator it = find(index);
            if (it != end())
                item = (*it).obj;
            return item;
        }
        void insertItem(int index, QObject *obj) {
            insert(index, ObjectRef(obj));
        }
        bool releaseItem(QObject *obj) {
            QHash<int, ObjectRef>::iterator it = begin();
            for (; it != end(); ++it) {
                ObjectRef &objRef = *it;
                if (objRef.obj == obj) {
                    if (--objRef.ref == 0) {
                        erase(it);
                        return true;
                    }
                    break;
                }
            }
            return false;
        }
    };

    int modelCount() const {
        if (m_visualItemModel)
            return m_visualItemModel->count();
        if (m_listModelInterface)
            return m_listModelInterface->count();
        if (m_abstractItemModel)
            return m_abstractItemModel->rowCount(m_root);
        if (m_listAccessor)
            return m_listAccessor->count();
        return 0;
    }

    Cache m_cache;
    QHash<QObject *, QDeclarativePackage*> m_packaged;

    QDeclarativeVisualDataModelParts *m_parts;
    friend class QDeclarativeVisualItemParts;

    VDMDelegateDataType *m_delegateDataType;
    friend class QDeclarativeVisualDataModelData;
    bool m_metaDataCreated : 1;
    bool m_metaDataCacheable : 1;
    bool m_delegateValidated : 1;
    bool m_completePending : 1;

    QDeclarativeVisualDataModelData *data(QObject *item);

    QVariant m_modelVariant;
    QDeclarativeListAccessor *m_listAccessor;

    QModelIndex m_root;
};

class QDeclarativeVisualDataModelDataMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QDeclarativeVisualDataModelDataMetaObject(QObject *parent, QDeclarativeOpenMetaObjectType *type)
    : QDeclarativeOpenMetaObject(parent, type) {}

    virtual QVariant initialValue(int);
    virtual int createProperty(const char *, const char *);

private:
    friend class QDeclarativeVisualDataModelData;
};

class QDeclarativeVisualDataModelData : public QObject
{
Q_OBJECT
public:
    QDeclarativeVisualDataModelData(int index, QDeclarativeVisualDataModel *model);
    ~QDeclarativeVisualDataModelData();

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() const;
    void setIndex(int index);

    int propForRole(int) const;
    void setValue(int, const QVariant &);
    bool hasValue(int id) const {
        return m_meta->hasValue(id);
    }

    void ensureProperties();

Q_SIGNALS:
    void indexChanged();

private:
    friend class QDeclarativeVisualDataModelDataMetaObject;
    int m_index;
    QDeclarativeGuard<QDeclarativeVisualDataModel> m_model;
    QDeclarativeVisualDataModelDataMetaObject *m_meta;
};

int QDeclarativeVisualDataModelData::propForRole(int id) const
{
    QDeclarativeVisualDataModelPrivate *model = QDeclarativeVisualDataModelPrivate::get(m_model);
    QHash<int,int>::const_iterator it = model->roleToPropId.find(id);
    if (it != model->roleToPropId.end())
        return *it;

    return -1;
}

void QDeclarativeVisualDataModelData::setValue(int id, const QVariant &val)
{
    m_meta->setValue(id, val);
}

int QDeclarativeVisualDataModelDataMetaObject::createProperty(const char *name, const char *type)
{
    QDeclarativeVisualDataModelData *data =
        static_cast<QDeclarativeVisualDataModelData *>(object());

    if (!data->m_model)
        return -1;

    QDeclarativeVisualDataModelPrivate *model = QDeclarativeVisualDataModelPrivate::get(data->m_model);
    if (data->m_index < 0 || data->m_index >= model->modelCount())
        return -1;

    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (model->m_listAccessor->type() == QDeclarativeListAccessor::ListProperty) {
            model->ensureRoles();
            if (qstrcmp(name,"modelData") == 0)
                return QDeclarativeOpenMetaObject::createProperty(name, type);
        }
    }
    return -1;
}

QVariant QDeclarativeVisualDataModelDataMetaObject::initialValue(int propId)
{
    QDeclarativeVisualDataModelData *data =
        static_cast<QDeclarativeVisualDataModelData *>(object());

    Q_ASSERT(data->m_model);
    QDeclarativeVisualDataModelPrivate *model = QDeclarativeVisualDataModelPrivate::get(data->m_model);

    QByteArray propName = name(propId);
    if ((!model->m_listModelInterface || !model->m_abstractItemModel) && model->m_listAccessor) {
        if (propName == "modelData") {
            if (model->m_listAccessor->type() == QDeclarativeListAccessor::Instance) {
                QObject *object = model->m_listAccessor->at(0).value<QObject*>();
                return object->metaObject()->property(1).read(object); // the first property after objectName
            }
            return model->m_listAccessor->at(data->m_index);
        } else {
            // return any property of a single object instance.
            QObject *object = model->m_listAccessor->at(data->m_index).value<QObject*>();
            return object->property(propName);
        }
    } else if (model->m_listModelInterface) {
        model->ensureRoles();
        QHash<QByteArray,int>::const_iterator it = model->m_roleNames.find(propName);
        if (it != model->m_roleNames.end()) {
            QVariant value = model->m_listModelInterface->data(data->m_index, *it);
            return value;
        } else if (model->m_roles.count() == 1 && propName == "modelData") {
            //for compatibility with other lists, assign modelData if there is only a single role
            QVariant value = model->m_listModelInterface->data(data->m_index, model->m_roles.first());
            return value;
        }
    } else if (model->m_abstractItemModel) {
        model->ensureRoles();
        if (propName == "hasModelChildren") {
            QModelIndex index = model->m_abstractItemModel->index(data->m_index, 0, model->m_root);
            return model->m_abstractItemModel->hasChildren(index);
        } else {
            QHash<QByteArray,int>::const_iterator it = model->m_roleNames.find(propName);
            if (it != model->m_roleNames.end()) {
                QModelIndex index = model->m_abstractItemModel->index(data->m_index, 0, model->m_root);
                return model->m_abstractItemModel->data(index, *it);
            }
        }
    }
    Q_ASSERT(!"Can never be reached");
    return QVariant();
}

QDeclarativeVisualDataModelData::QDeclarativeVisualDataModelData(int index,
                                               QDeclarativeVisualDataModel *model)
: m_index(index), m_model(model),
m_meta(new QDeclarativeVisualDataModelDataMetaObject(this, QDeclarativeVisualDataModelPrivate::get(model)->m_delegateDataType))
{
    ensureProperties();
}

QDeclarativeVisualDataModelData::~QDeclarativeVisualDataModelData()
{
}

void QDeclarativeVisualDataModelData::ensureProperties()
{
    QDeclarativeVisualDataModelPrivate *modelPriv = QDeclarativeVisualDataModelPrivate::get(m_model);
    if (modelPriv->m_metaDataCacheable && !modelPriv->m_metaDataCreated) {
        modelPriv->createMetaData();
        if (modelPriv->m_metaDataCreated)
            m_meta->setCached(true);
    }
}

int QDeclarativeVisualDataModelData::index() const
{
    return m_index;
}

// This is internal only - it should not be set from qml
void QDeclarativeVisualDataModelData::setIndex(int index)
{
    m_index = index;
    emit indexChanged();
}

//---------------------------------------------------------------------------

class QDeclarativeVisualDataModelPartsMetaObject : public QDeclarativeOpenMetaObject
{
public:
    QDeclarativeVisualDataModelPartsMetaObject(QObject *parent)
    : QDeclarativeOpenMetaObject(parent) {}

    virtual void propertyCreated(int, QMetaPropertyBuilder &);
    virtual QVariant initialValue(int);
};

class QDeclarativeVisualDataModelParts : public QObject
{
Q_OBJECT
public:
    QDeclarativeVisualDataModelParts(QDeclarativeVisualDataModel *parent);

private:
    friend class QDeclarativeVisualDataModelPartsMetaObject;
    QDeclarativeVisualDataModel *model;
};

void QDeclarativeVisualDataModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QDeclarativeVisualDataModelPartsMetaObject::initialValue(int id)
{
    QDeclarativeVisualDataModel *m = new QDeclarativeVisualDataModel;
    m->setParent(object());
    m->setPart(QString::fromUtf8(name(id)));
    m->setModel(QVariant::fromValue(static_cast<QDeclarativeVisualDataModelParts *>(object())->model));

    QVariant var = QVariant::fromValue((QObject *)m);
    return var;
}

QDeclarativeVisualDataModelParts::QDeclarativeVisualDataModelParts(QDeclarativeVisualDataModel *parent)
: QObject(parent), model(parent)
{
    new QDeclarativeVisualDataModelPartsMetaObject(this);
}

QDeclarativeVisualDataModelPrivate::QDeclarativeVisualDataModelPrivate(QDeclarativeContext *ctxt)
: m_listModelInterface(0), m_abstractItemModel(0), m_visualItemModel(0), m_delegate(0)
, m_context(ctxt), m_parts(0), m_delegateDataType(0), m_metaDataCreated(false)
, m_metaDataCacheable(false), m_delegateValidated(false), m_completePending(false), m_listAccessor(0)
{
}

QDeclarativeVisualDataModelData *QDeclarativeVisualDataModelPrivate::data(QObject *item)
{
    QDeclarativeVisualDataModelData *dataItem =
        item->findChild<QDeclarativeVisualDataModelData *>();
    Q_ASSERT(dataItem);
    return dataItem;
}

//---------------------------------------------------------------------------

/*!
    \qmlclass VisualDataModel QDeclarativeVisualDataModel
    \ingroup qml-working-with-data
    \brief The VisualDataModel encapsulates a model and delegate

    A VisualDataModel encapsulates a model and the delegate that will
    be instantiated for items in the model.

    It is usually not necessary to create VisualDataModel elements.
    However, it can be useful for manipulating and accessing the \l modelIndex 
    when a QAbstractItemModel subclass is used as the 
    model. Also, VisualDataModel is used together with \l Package to 
    provide delegates to multiple views.

    The example below illustrates using a VisualDataModel with a ListView.

    \snippet doc/src/snippets/declarative/visualdatamodel.qml 0
*/

QDeclarativeVisualDataModel::QDeclarativeVisualDataModel()
: QDeclarativeVisualModel(*(new QDeclarativeVisualDataModelPrivate(0)))
{
}

QDeclarativeVisualDataModel::QDeclarativeVisualDataModel(QDeclarativeContext *ctxt, QObject *parent)
: QDeclarativeVisualModel(*(new QDeclarativeVisualDataModelPrivate(ctxt)), parent)
{
}

QDeclarativeVisualDataModel::~QDeclarativeVisualDataModel()
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_listAccessor)
        delete d->m_listAccessor;
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
}

/*!
    \qmlproperty model VisualDataModel::model
    This property holds the model providing data for the VisualDataModel.

    The model provides a set of data that is used to create the items
    for a view.  For large or dynamic datasets the model is usually
    provided by a C++ model object.  The C++ model object must be a \l
    {QAbstractItemModel} subclass or a simple list.

    Models can also be created directly in QML, using a \l{ListModel} or
    \l{XmlListModel}.

    \sa {qmlmodels}{Data Models}
*/
QVariant QDeclarativeVisualDataModel::model() const
{
    Q_D(const QDeclarativeVisualDataModel);
    return d->m_modelVariant;
}

void QDeclarativeVisualDataModel::setModel(const QVariant &model)
{
    Q_D(QDeclarativeVisualDataModel);
    delete d->m_listAccessor;
    d->m_listAccessor = 0;
    d->m_modelVariant = model;
    if (d->m_listModelInterface) {
        // Assume caller has released all items.
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsChanged(int,int,QList<int>)),
                this, SLOT(_q_itemsChanged(int,int,QList<int>)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsInserted(int,int)),
                this, SLOT(_q_itemsInserted(int,int)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsRemoved(int,int)),
                this, SLOT(_q_itemsRemoved(int,int)));
        QObject::disconnect(d->m_listModelInterface, SIGNAL(itemsMoved(int,int,int)),
                this, SLOT(_q_itemsMoved(int,int,int)));
        d->m_listModelInterface = 0;
    } else if (d->m_abstractItemModel) {
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsInserted(const QModelIndex &,int,int)),
                            this, SLOT(_q_rowsInserted(const QModelIndex &,int,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsRemoved(const QModelIndex &,int,int)),
                            this, SLOT(_q_rowsRemoved(const QModelIndex &,int,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
                            this, SLOT(_q_dataChanged(const QModelIndex&,const QModelIndex&)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
                            this, SLOT(_q_rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)));
        QObject::disconnect(d->m_abstractItemModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));
    } else if (d->m_visualItemModel) {
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsInserted(int,int)),
                         this, SIGNAL(itemsInserted(int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsRemoved(int,int)),
                         this, SIGNAL(itemsRemoved(int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(itemsMoved(int,int,int)),
                         this, SIGNAL(itemsMoved(int,int,int)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarativePackage*)),
                         this, SLOT(_q_createdPackage(int,QDeclarativePackage*)));
        QObject::disconnect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarativePackage*)),
                         this, SLOT(_q_destroyingPackage(QDeclarativePackage*)));
        d->m_visualItemModel = 0;
    }

    d->m_roles.clear();
    d->m_roleNames.clear();
    if (d->m_delegateDataType)
        d->m_delegateDataType->release();
    d->m_metaDataCreated = 0;
    d->m_metaDataCacheable = false;
    d->m_delegateDataType = new VDMDelegateDataType(&QDeclarativeVisualDataModelData::staticMetaObject, d->m_context?d->m_context->engine():qmlEngine(this));

    QObject *object = qvariant_cast<QObject *>(model);
    if (object && (d->m_listModelInterface = qobject_cast<QListModelInterface *>(object))) {
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsChanged(int,int,QList<int>)),
                         this, SLOT(_q_itemsChanged(int,int,QList<int>)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsInserted(int,int)),
                         this, SLOT(_q_itemsInserted(int,int)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsRemoved(int,int)),
                         this, SLOT(_q_itemsRemoved(int,int)));
        QObject::connect(d->m_listModelInterface, SIGNAL(itemsMoved(int,int,int)),
                         this, SLOT(_q_itemsMoved(int,int,int)));
        d->m_metaDataCacheable = true;
        if (d->m_delegate && d->m_listModelInterface->count())
            emit itemsInserted(0, d->m_listModelInterface->count());
        return;
    } else if (object && (d->m_abstractItemModel = qobject_cast<QAbstractItemModel *>(object))) {
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsInserted(const QModelIndex &,int,int)),
                            this, SLOT(_q_rowsInserted(const QModelIndex &,int,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsRemoved(const QModelIndex &,int,int)),
                            this, SLOT(_q_rowsRemoved(const QModelIndex &,int,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(dataChanged(const QModelIndex&,const QModelIndex&)),
                            this, SLOT(_q_dataChanged(const QModelIndex&,const QModelIndex&)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)),
                            this, SLOT(_q_rowsMoved(const QModelIndex&,int,int,const QModelIndex&,int)));
        QObject::connect(d->m_abstractItemModel, SIGNAL(modelReset()), this, SLOT(_q_modelReset()));
        d->m_metaDataCacheable = true;
        return;
    }
    if ((d->m_visualItemModel = qvariant_cast<QDeclarativeVisualDataModel *>(model))) {
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsInserted(int,int)),
                         this, SIGNAL(itemsInserted(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsRemoved(int,int)),
                         this, SIGNAL(itemsRemoved(int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(itemsMoved(int,int,int)),
                         this, SIGNAL(itemsMoved(int,int,int)));
        QObject::connect(d->m_visualItemModel, SIGNAL(createdPackage(int,QDeclarativePackage*)),
                         this, SLOT(_q_createdPackage(int,QDeclarativePackage*)));
        QObject::connect(d->m_visualItemModel, SIGNAL(destroyingPackage(QDeclarativePackage*)),
                         this, SLOT(_q_destroyingPackage(QDeclarativePackage*)));
        return;
    }
    d->m_listAccessor = new QDeclarativeListAccessor;
    d->m_listAccessor->setList(model, d->m_context?d->m_context->engine():qmlEngine(this));
    if (d->m_listAccessor->type() != QDeclarativeListAccessor::ListProperty)
        d->m_metaDataCacheable = true;
    if (d->m_delegate && d->modelCount()) {
        emit itemsInserted(0, d->modelCount());
        emit countChanged();
    }
}

/*!
    \qmlproperty Component VisualDataModel::delegate

    The delegate provides a template defining each item instantiated by a view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qmlmodels}{Data Model}.
*/
QDeclarativeComponent *QDeclarativeVisualDataModel::delegate() const
{
    Q_D(const QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->delegate();
    return d->m_delegate;
}

void QDeclarativeVisualDataModel::setDelegate(QDeclarativeComponent *delegate)
{
    Q_D(QDeclarativeVisualDataModel);
    bool wasValid = d->m_delegate != 0;
    d->m_delegate = delegate;
    d->m_delegateValidated = false;
    if (!wasValid && d->modelCount() && d->m_delegate) {
        emit itemsInserted(0, d->modelCount());
        emit countChanged();
    }
    if (wasValid && !d->m_delegate && d->modelCount()) {
        emit itemsRemoved(0, d->modelCount());
        emit countChanged();
    }
}

/*!
    \qmlproperty QModelIndex VisualDataModel::rootIndex

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  \c rootIndex allows the children of
    any node in a QAbstractItemModel to be provided by this model.

    This property only affects models of type QAbstractItemModel.

    For example, here is a simple interactive file system browser.
    When a directory name is clicked, the view's \c rootIndex is set to the
    QModelIndex node of the clicked directory, thus updating the view to show
    the new directory's contents.

    \c main.cpp:
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/main.cpp 0
   
    \c view.qml:
    \snippet doc/src/snippets/declarative/visualdatamodel_rootindex/view.qml 0

    If the \l model is a QAbstractItemModel subclass, the delegate can also
    reference a \c hasModelChildren property (optionally qualified by a
    \e model. prefix) that indicates whether the delegate's model item has 
    any child nodes.


    \sa modelIndex(), parentModelIndex()
*/
QVariant QDeclarativeVisualDataModel::rootIndex() const
{
    Q_D(const QDeclarativeVisualDataModel);
    return QVariant::fromValue(d->m_root);
}

void QDeclarativeVisualDataModel::setRootIndex(const QVariant &root)
{
    Q_D(QDeclarativeVisualDataModel);
    QModelIndex modelIndex = qvariant_cast<QModelIndex>(root);
    if (d->m_root != modelIndex) {
        int oldCount = d->modelCount();
        d->m_root = modelIndex;
        int newCount = d->modelCount();
        if (d->m_delegate && oldCount)
            emit itemsRemoved(0, oldCount);
        if (d->m_delegate && newCount)
            emit itemsInserted(0, newCount);
        if (newCount != oldCount)
            emit countChanged();
        emit rootIndexChanged();
    }
}


/*!
    \qmlmethod QModelIndex VisualDataModel::modelIndex(int index)

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the specified index.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QDeclarativeVisualDataModel::modelIndex(int idx) const
{
    Q_D(const QDeclarativeVisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->index(idx, 0, d->m_root));
    return QVariant::fromValue(QModelIndex());
}

/*!
    \qmlmethod QModelIndex VisualDataModel::parentModelIndex()

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the parent of the current rootIndex.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QDeclarativeVisualDataModel::parentModelIndex() const
{
    Q_D(const QDeclarativeVisualDataModel);
    if (d->m_abstractItemModel)
        return QVariant::fromValue(d->m_abstractItemModel->parent(d->m_root));
    return QVariant::fromValue(QModelIndex());
}

QString QDeclarativeVisualDataModel::part() const
{
    Q_D(const QDeclarativeVisualDataModel);
    return d->m_part;
}

void QDeclarativeVisualDataModel::setPart(const QString &part)
{
    Q_D(QDeclarativeVisualDataModel);
    d->m_part = part;
}

int QDeclarativeVisualDataModel::count() const
{
    Q_D(const QDeclarativeVisualDataModel);
    return d->modelCount();
}

QDeclarativeItem *QDeclarativeVisualDataModel::item(int index, bool complete)
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->item(index, d->m_part.toUtf8(), complete);
    return item(index, QByteArray(), complete);
}

/*
  Returns ReleaseStatus flags.
*/
QDeclarativeVisualDataModel::ReleaseFlags QDeclarativeVisualDataModel::release(QDeclarativeItem *item)
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->release(item);

    ReleaseFlags stat = 0;
    QObject *obj = item;
    bool inPackage = false;

    QHash<QObject*,QDeclarativePackage*>::iterator it = d->m_packaged.find(item);
    if (it != d->m_packaged.end()) {
        QDeclarativePackage *package = *it;
        d->m_packaged.erase(it);
        if (d->m_packaged.contains(item))
            stat |= Referenced;
        inPackage = true;
        obj = package; // fall through and delete
    }

    if (d->m_cache.releaseItem(obj)) {
        // Remove any bindings to avoid warnings due to parent change.
        QObjectPrivate *p = QObjectPrivate::get(obj);
        Q_ASSERT(p->declarativeData);
        QDeclarativeData *d = static_cast<QDeclarativeData*>(p->declarativeData);
        if (d->ownContext && d->context)
            d->context->clearExpressions();

        if (inPackage) {
            emit destroyingPackage(qobject_cast<QDeclarativePackage*>(obj));
        } else {
            if (item->scene())
                item->scene()->removeItem(item);
        }
        stat |= Destroyed;
        obj->deleteLater();
    } else if (!inPackage) {
        stat |= Referenced;
    }

    return stat;
}

/*!
    \qmlproperty object VisualDataModel::parts

    The \a parts property selects a VisualDataModel which creates
    delegates from the part named.  This is used in conjunction with
    the \l Package element.

    For example, the code below selects a model which creates
    delegates named \e list from a \l Package:

    \code
    VisualDataModel {
        id: visualModel
        delegate: Package {
            Item { Package.name: "list" }
        }
        model: myModel
    }

    ListView {
        width: 200; height:200
        model: visualModel.parts.list
    }
    \endcode

    \sa Package
*/
QObject *QDeclarativeVisualDataModel::parts()
{
    Q_D(QDeclarativeVisualDataModel);
    if (!d->m_parts)
        d->m_parts = new QDeclarativeVisualDataModelParts(this);
    return d->m_parts;
}

QDeclarativeItem *QDeclarativeVisualDataModel::item(int index, const QByteArray &viewId, bool complete)
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->item(index, viewId, complete);

    if (d->modelCount() <= 0 || !d->m_delegate)
        return 0;
    QObject *nobj = d->m_cache.getItem(index);
    bool needComplete = false;
    if (!nobj) {
        QDeclarativeContext *ccontext = d->m_context;
        if (!ccontext) ccontext = qmlContext(this);
        QDeclarativeContext *ctxt = new QDeclarativeContext(ccontext);
        QDeclarativeVisualDataModelData *data = new QDeclarativeVisualDataModelData(index, this);
        if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor
            && d->m_listAccessor->type() == QDeclarativeListAccessor::ListProperty) {
            ctxt->setContextObject(d->m_listAccessor->at(index).value<QObject*>());
            ctxt = new QDeclarativeContext(ctxt, ctxt);
        }
        ctxt->setContextProperty(QLatin1String("model"), data);
        ctxt->setContextObject(data);
        d->m_completePending = false;
        nobj = d->m_delegate->beginCreate(ctxt);
        if (complete) {
            d->m_delegate->completeCreate();
        } else {
            d->m_completePending = true;
            needComplete = true;
        }
        if (nobj) {
            QDeclarative_setParent_noEvent(ctxt, nobj);
            QDeclarative_setParent_noEvent(data, nobj);
            d->m_cache.insertItem(index, nobj);
            if (QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(nobj))
                emit createdPackage(index, package);
        } else {
            delete data;
            delete ctxt;
            qmlInfo(this, d->m_delegate->errors()) << "Error creating delgate";
        }
    }
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(nobj);
    if (!item) {
        QDeclarativePackage *package = qobject_cast<QDeclarativePackage *>(nobj);
        if (package) {
            QObject *o = package->part(QString::fromUtf8(viewId));
            item = qobject_cast<QDeclarativeItem *>(o);
            if (item)
                d->m_packaged.insertMulti(item, package);
        }
    }
    if (!item) {
        if (needComplete)
            d->m_delegate->completeCreate();
        d->m_cache.releaseItem(nobj);
        if (!d->m_delegateValidated) {
            qmlInfo(d->m_delegate) << QDeclarativeVisualDataModel::tr("Delegate component must be Item type.");
            d->m_delegateValidated = true;
        }
    }

    return item;
}

bool QDeclarativeVisualDataModel::completePending() const
{
    Q_D(const QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->completePending();
    return d->m_completePending;
}

void QDeclarativeVisualDataModel::completeItem()
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel) {
        d->m_visualItemModel->completeItem();
        return;
    }

    d->m_delegate->completeCreate();
    d->m_completePending = false;
}

QString QDeclarativeVisualDataModel::stringValue(int index, const QString &name)
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->stringValue(index, name);

    if ((!d->m_listModelInterface || !d->m_abstractItemModel) && d->m_listAccessor) {
        if (QObject *object = d->m_listAccessor->at(index).value<QObject*>())
            return object->property(name.toUtf8()).toString();
    }

    if ((!d->m_listModelInterface && !d->m_abstractItemModel) || !d->m_delegate)
        return QString();

    QString val;
    QObject *data = 0;
    bool tempData = false;

    if (QObject *nobj = d->m_cache.item(index))
        data = d->data(nobj);
    if (!data) {
        data = new QDeclarativeVisualDataModelData(index, this);
        tempData = true;
    }

    QDeclarativeData *ddata = QDeclarativeData::get(data);
    if (ddata && ddata->propertyCache) {
        QDeclarativePropertyCache::Data *prop = ddata->propertyCache->property(name);
        if (prop) {
            if (prop->propType == QVariant::String) {
                void *args[] = { &val, 0 };
                QMetaObject::metacall(data, QMetaObject::ReadProperty, prop->coreIndex, args);
            } else if (prop->propType == qMetaTypeId<QVariant>()) {
                QVariant v;
                void *args[] = { &v, 0 };
                QMetaObject::metacall(data, QMetaObject::ReadProperty, prop->coreIndex, args);
                val = v.toString();
            }
        } else {
            val = data->property(name.toUtf8()).toString();
        }
    } else {
        val = data->property(name.toUtf8()).toString();
    }

    if (tempData)
        delete data;

    return val;
}

QVariant QDeclarativeVisualDataModel::evaluate(int index, const QString &expression, QObject *objectContext)
{
    Q_D(QDeclarativeVisualDataModel);
    if (d->m_visualItemModel)
        return d->m_visualItemModel->evaluate(index, expression, objectContext);

    if ((!d->m_listModelInterface && !d->m_abstractItemModel) || !d->m_delegate)
        return QVariant();

    QVariant value;
    QObject *nobj = d->m_cache.item(index);
    if (nobj) {
        QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(nobj);
        if (item) {
            QDeclarativeExpression e(qmlContext(item), objectContext, expression);
            value = e.evaluate();
        }
    } else {
        QDeclarativeContext *ccontext = d->m_context;
        if (!ccontext) ccontext = qmlContext(this);
        QDeclarativeContext *ctxt = new QDeclarativeContext(ccontext);
        QDeclarativeVisualDataModelData *data = new QDeclarativeVisualDataModelData(index, this);
        ctxt->setContextObject(data);
        QDeclarativeExpression e(ctxt, objectContext, expression);
        value = e.evaluate();
        delete data;
        delete ctxt;
    }

    return value;
}

int QDeclarativeVisualDataModel::indexOf(QDeclarativeItem *item, QObject *) const
{
    QVariant val = QDeclarativeEngine::contextForObject(item)->contextProperty(QLatin1String("index"));
        return val.toInt();
    return -1;
}

void QDeclarativeVisualDataModel::_q_itemsChanged(int index, int count,
                                         const QList<int> &roles)
{
    Q_D(QDeclarativeVisualDataModel);
    for (QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef>::ConstIterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ++iter) {
        const int idx = iter.key();

        if (idx >= index && idx < index+count) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            for (int roleIdx = 0; roleIdx < roles.count(); ++roleIdx) {
                int role = roles.at(roleIdx);
                int propId = data->propForRole(role);
                if (propId != -1) {
                    if (data->hasValue(propId)) {
                        if (d->m_listModelInterface) {
                            data->setValue(propId, d->m_listModelInterface->data(idx, QList<int>() << role).value(role));
                        } else if (d->m_abstractItemModel) {
                            QModelIndex index = d->m_abstractItemModel->index(idx, 0, d->m_root);
                            data->setValue(propId, d->m_abstractItemModel->data(index, role));
                        }
                    }
                } else {
                    QString roleName;
                    if (d->m_listModelInterface)
                        roleName = d->m_listModelInterface->toString(role);
                    else if (d->m_abstractItemModel)
                        roleName = QString::fromUtf8(d->m_abstractItemModel->roleNames().value(role));
                    qmlInfo(this) << "Changing role not present in item: " << roleName;
                }
            }
        }
    }
}

void QDeclarativeVisualDataModel::_q_itemsInserted(int index, int count)
{
    Q_D(QDeclarativeVisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= index) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + count;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsInserted(index, count);
    emit countChanged();
}

void QDeclarativeVisualDataModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QDeclarativeVisualDataModel);
    if (!count)
        return;
    // XXX - highly inefficient
    QHash<int, QDeclarativeVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int, QDeclarativeVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {
        if (iter.key() >= index && iter.key() < index + count) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            iter = d->m_cache.erase(iter);
            items.insertMulti(-1, objRef); //XXX perhaps better to maintain separately
            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(-1);
        } else if (iter.key() >= index + count) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - count;
            iter = d->m_cache.erase(iter);
            items.insert(index, objRef);
            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }

    d->m_cache.unite(items);
    emit itemsRemoved(index, count);
    emit countChanged();
}

void QDeclarativeVisualDataModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QDeclarativeVisualDataModel);
    // XXX - highly inefficient
    QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef> items;
    for (QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        if (iter.key() >= from && iter.key() < from + count) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() - from + to;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    for (QHash<int,QDeclarativeVisualDataModelPrivate::ObjectRef>::Iterator iter = d->m_cache.begin();
        iter != d->m_cache.end(); ) {

        int diff = from > to ? count : -count;
        if (iter.key() >= qMin(from,to) && iter.key() < qMax(from+count,to+count)) {
            QDeclarativeVisualDataModelPrivate::ObjectRef objRef = *iter;
            int index = iter.key() + diff;
            iter = d->m_cache.erase(iter);

            items.insert(index, objRef);

            QDeclarativeVisualDataModelData *data = d->data(objRef.obj);
            data->setIndex(index);
        } else {
            ++iter;
        }
    }
    d->m_cache.unite(items);

    emit itemsMoved(from, to, count);
}

void QDeclarativeVisualDataModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    if (!parent.isValid())
        _q_itemsInserted(begin, end - begin + 1);
}

void QDeclarativeVisualDataModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    if (!parent.isValid())
        _q_itemsRemoved(begin, end - begin + 1);
}

void QDeclarativeVisualDataModel::_q_rowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
{
    const int count = sourceEnd - sourceStart + 1;
    if (!destinationParent.isValid() && !sourceParent.isValid()) {
        _q_itemsMoved(sourceStart, destinationRow, count);
    } else if (!sourceParent.isValid()) {
        _q_itemsRemoved(sourceStart, count);
    } else if (!destinationParent.isValid()) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QDeclarativeVisualDataModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    Q_D(QDeclarativeVisualDataModel);
    if (!begin.parent().isValid())
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, d->m_roles);
}

void QDeclarativeVisualDataModel::_q_modelReset()
{
    emit modelReset();
}

void QDeclarativeVisualDataModel::_q_createdPackage(int index, QDeclarativePackage *package)
{
    Q_D(QDeclarativeVisualDataModel);
    emit createdItem(index, qobject_cast<QDeclarativeItem*>(package->part(d->m_part)));
}

void QDeclarativeVisualDataModel::_q_destroyingPackage(QDeclarativePackage *package)
{
    Q_D(QDeclarativeVisualDataModel);
    emit destroyingItem(qobject_cast<QDeclarativeItem*>(package->part(d->m_part)));
}

QT_END_NAMESPACE

QML_DECLARE_TYPE(QListModelInterface)

#include <qdeclarativevisualitemmodel.moc>