
#include "shvarmodel.h"

ShVarModel::ShVarModel(QObject *parent) :
	QStandardItemModel(parent)
{
	QStringList column_names;
	column_names << "Name" << "FullName" << "Type" << "TypeID" << "Qualifier"
			<< "ID" << "CgblType" << "CgblIndexA" << "CgblIndexB" << "Built-In"
			<< "Changed" << "Scope" << "Selectable" << "Watched"
			<< "Data PixelBox" << "Data VertexBox" << "Data CurrentBox"
			<< "Selection Value" << "Value";

	this->setVerticalHeaderLabels(column_names);	
}

void ShVarModel::appendRow(const ShVariableList *items)
{
	QStandardItem *root = this->invisibleRootItem();
	for (i = 0; i < i_pVL->numVariables; i++) {
		ShVarItem* item = new ShVarItem(i_pVL->variables[i]);
		root->appendRow(item);
	}
}

void ShVarModel::setRecursive(QVariant data, varDataFields field, ShVarItem *item)
{
	if (!item)
		return;

	if (item != this->invisibleRootItem()){
		item->setData(data, field);
		emit dataChanged(item->index(), item->index());
	}

	for (int row = 0; row < item->rowCount(); ++row)
		setRecursive(field, data, item->child(row));
}

void ShVarModel::setWatched(ShVarItem *item)
{
	if (!item)
		return;

	if (item->data(DF_SELECTABLE).toBool() && (item->rowCount() > 0
				|| !item->data(DF_WATCHED).toBool())) {

		if (item->rowCount() < 1) {
			setRecursive(true, DF_WATCHED, item);
			//m_qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			emit addWatchItem(item);
		} else {
			for (int i = 0; i < item->rowCount(); i++)
				this->setWatched(item->child(i));
		}
	}
}

void ShVarModel::unsetWatched(ShVarItem *item)
{
	if (!item)
		return;

	for (int i = 0; i < item->rowCount(); ++i)
		unsetWatched(item->child(i)->index());

	/*
	if (item->getPixelBoxPointer() != NULL) {
		PixelBox *fb = item->getPixelBoxPointer();
		item->setPixelBoxPointer(NULL);
		delete fb;
	}
	if (item->getCurrentPointer() != NULL) {
		VertexBox *vb = item->getCurrentPointer();
		item->setCurrentPointer(NULL);
		delete vb;
	}
	if (item->getVertexBoxPointer() != NULL) {
		VertexBox *vb = item->getVertexBoxPointer();
		item->setVertexBoxPointer(NULL);
		delete vb;
	} */

	item->setData(false, DF_WATCHED);
	emit dataChanged(item->index(), item->index());
	if (item->parent() != this->invisibleRootItem())
		unsetWatched(item->parent()->index());
}

void ShVarModel::setChangedAndScope(ShChangeableList &cl, DbgRsScope &scope, DbgRsScopeStack &stack)
{
	QStandardItem *root = this->invisibleRootItem();

	setChanged(false, root);
	dumpShChangeableList(&cl);


	/* Changed? */
	for (int i = 0; i < cl.numChangeables; i++) {
		ShChangeable *c = cl.changeables[i];

		for (int j = 0; j < root->rowCount(); j++) {
			ShVarItem *item = root->child(j);
			int id = item->data(DF_UNIQUE_ID).toInt();
			if (id != c->id)
				continue;

			item->setData(true, DF_CHANGED);
			for (int l = 0; l < c->numIndices; l++) {
				ShChangeableIndex *idx = c->indices[l];
				if (idx->type == SH_CGB_SWIZZLE) {
					// FIXME: only 4?
					for (int m = 0; m < 4; m++) {
						if ((idx->index & (1 << m)) == (1 << m))
							item->child(m)->setData(true, DF_CHANGED);
					}
					item = NULL;
				} else {
					/* SH_CGB_ARRAY, SH_CGB_STRUCT */
					if (idx->index == -1) {
						item->setData(true, DF_CHANGED);
						break;
					} else {
						if (item->data(DF_ARRAYTYPE).toInt() == SHV_MATRIX) {
							Q_ASSERT_X((c->numIndices - l) >= 2, "Changeable indices", "wrong");
							ShChangeableIndex *subidx = c->indices[++l];
							item = item->child(idx->index)->child(subidx->index);
							item->setData(true, DF_CHANGED);
						} else {
							item = item->child(idx->index);
							item->setData(true, DF_CHANGED);
						}
					}

				}
			}
			setRecursive(true, DF_CHANGED, item);
		}
	}

	/* check scope */
	for (int j = 0; j < root->rowCount(); j++) {
		ShVarItem *item = root->child(j);
		if (item->data(DF_BUILTIN).toBool())
			continue;

		int id = item->data(DF_UNIQUE_ID).toInt();
		ShVarItem::Scope old = (ShVarItem::Scope)item->data(DF_SCOPE).toInt();
		/* check scope list */
		int i;
		for (i = 0; i < scope.numIds; i++) {
			if (id != scope.ids[i])
				continue;
			if (old == ShVarItem::NewInScope)
				setRecursive(ShVarItem::InScope, DF_SCOPE, item);
			else if (old != ShVarItem::InScope)
				setRecursive(ShVarItem::NewInScope, DF_SCOPE, item);
			break;
		}
		/* id was in scope list */
		if (i < scope.numIds)
			continue;

		/* not in scope list */
		/* check scope stack */
		for (i = 0; i < stack.numIds; i++) {
			if (id != stack.ids[i])
				continue;
			setRecursive(ShVarItem::InScopeStack, DF_SCOPE, item);
			break;
		}
		/* id was in scope stack */
		if (i < stack.numIds)
			continue;

		/* not in scope stack */
		if (old == ShVarItem::LeftScope)
			setRecursive(ShVarItem::OutOfScope, DF_SCOPE, item);
		else
			setRecursive(ShVarItem::LeftScope, DF_SCOPE, item);
	}

	ShVarItem *item;
	item = root->child(0);
	QModelIndex indexBegin = index(item->row(), 0);
	item = root->child(root->rowCount() - 1);
	QModelIndex indexEnd = index(item->row(), 0);
	emit dataChanged(indexBegin, indexEnd);
}
