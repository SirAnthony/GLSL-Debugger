
#ifndef SHTABLEDATAMODEL_H
#define SHTABLEDATAMODEL_H

class ShVarItem;

class ShTableDataModel
{
public:
	explicit ShTableDataModel();
	virtual bool addItem(ShVarItem *) = 0;
	
};

#endif // SHTABLEDATAMODEL_H
