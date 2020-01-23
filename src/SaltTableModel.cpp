/*
 * SaltTableModel.cpp is part of Brewtarget, and is Copyright the following
 * authors 2009-2014
 * - Mik Firestone <mikfire@gmail.com>
 * - Philip Greggory Lee <rocketman768@gmail.com>
 * - swstim <swstim@gmail.com>
 *
 * Brewtarget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Brewtarget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QAbstractTableModel>
#include <QAbstractItemModel>
#include <QWidget>
#include <QComboBox>
#include <QHeaderView>
#include <QModelIndex>
#include <QVariant>
#include <QItemDelegate>
#include <QStyleOptionViewItem>
#include <QLineEdit>
#include <QString>
#include <QObject>

#include <QList>
#include "database.h"
#include "WaterDialog.h"
#include "SaltTableModel.h"
#include "salt.h"
#include "unit.h"
#include "recipe.h"
#include "mash.h"
#include "brewtarget.h"

static QStringList addToName = QStringList() << QObject::tr("Never")
                                             << QObject::tr("Mash")
                                             << QObject::tr("Sparge")
                                             << QObject::tr("Ratio")
                                             << QObject::tr("Both");

static QStringList saltNames = QStringList() << QObject::tr("None")
                                             << QObject::tr("CaCl2")
                                             << QObject::tr("CaCO3")
                                             << QObject::tr("CaSO4")
                                             << QObject::tr("MgSO4")
                                             << QObject::tr("NaCl")
                                             << QObject::tr("NaHCO3");

SaltTableModel::SaltTableModel(QTableView* parent)
   : QAbstractTableModel(parent),
     recObs(nullptr),
     parentTableWidget(parent)
{
   saltObs.clear();
   setObjectName("saltTable");

   QHeaderView* headerView = parentTableWidget->horizontalHeader();
   headerView->setContextMenuPolicy(Qt::CustomContextMenu);
   parentTableWidget->setWordWrap(false);

   connect(headerView, &QWidget::customContextMenuRequested, this, &SaltTableModel::contextMenu);
}

SaltTableModel::~SaltTableModel()
{
   saltObs.clear();
}

void SaltTableModel::observeRecipe(Recipe* rec)
{
   if ( recObs ) {
      disconnect( recObs, nullptr, this, nullptr );
      removeAll();
   }

   recObs = rec;
   if ( recObs ) {
      QList<Salt*> salts = recObs->salts();
      connect( recObs, &BeerXMLElement::changed, this, &SaltTableModel::changed );
      if (salts.size() > 0 ) {
         addSalts( salts );
         emit newTotals();
      }
      if ( recObs->mash() ) {
         spargePct = recObs->mash()->totalSpargeAmount_l()/recObs->mash()->totalInfusionAmount_l();
      }
   }
}

void SaltTableModel::addSalt(Salt* salt)
{
   if( saltObs.contains(salt) ) {
      return;
   }

   beginInsertRows( QModelIndex(), saltObs.size(), saltObs.size() );
   saltObs.append(salt);
   connect( salt, &BeerXMLElement::changed, this, &SaltTableModel::changed );
   endInsertRows();

   if (parentTableWidget) {
      parentTableWidget->resizeColumnsToContents();
      parentTableWidget->resizeRowsToContents();
   }
}

void SaltTableModel::addSalts(QList<Salt*> salts)
{
   QList<Salt*> tmp;

   foreach (Salt* i, salts) {
      if( !saltObs.contains(i) )
         tmp.append(i);
   }

   int size = saltObs.size();
   if (size+tmp.size()) {
      beginInsertRows( QModelIndex(), size, size+tmp.size()-1 );
      saltObs.append(tmp);

      foreach (Salt* i, tmp) {
         connect( i, &BeerXMLElement::changed, this, &SaltTableModel::changed );
      }

      endInsertRows();
   }

   if( parentTableWidget ) {
      parentTableWidget->resizeColumnsToContents();
      parentTableWidget->resizeRowsToContents();
   }

}

void SaltTableModel::catchSalt()
{
   Salt* gaq = new Salt(QString(),true);
   addSalt(gaq);
}

double SaltTableModel::total_Ca() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      // If we are adding equal amounts to mash and sparge
      // then double what's shown
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      // If we are adding a proportional amount to both,
      // this should handle that math.
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->Ca();
   }
   return ret;
}

double SaltTableModel::total_Cl() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->Cl();
   }

   return ret;
}

double SaltTableModel::total_CO3() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      int mult = i->addTo() == Salt::RATIO ? 2 : 1;
      ret += mult * i->CO3();
   }

   return ret;
}

double SaltTableModel::total_HCO3() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->HCO3();
   }

   return ret;
}

double SaltTableModel::total_Mg() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->Mg();
   }

   return ret;
}

double SaltTableModel::total_Na() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->Na();
   }

   return ret;
}

double SaltTableModel::total_SO4() const
{
   double ret = 0;
   foreach(Salt* i, saltObs) {
      double mult  = 1;
      if ( i->addTo() == Salt::EQUAL ) {
         mult = 2;
      }
      else if ( i->addTo() == Salt::RATIO ) {
         mult = 1.0 + spargePct;
      }
      ret += mult * i->SO4();
   }

   return ret;
}

double SaltTableModel::total(Salt::Types type) const
{
   double ret = 0;
   if (type != Salt::NONE) {
      foreach(Salt* i, saltObs) {
         if ( i->type() == type && i->addTo() != Salt::NEVER) {
            double mult  = 1;
            if ( i->addTo() == Salt::EQUAL ) {
               mult = 2;
            }
            else if ( i->addTo() == Salt::RATIO ) {
               mult = 1.0 + spargePct;
            }
            ret += mult * i->amount();
         }
      }
   }
   return ret;
}

void SaltTableModel::removeSalt(Salt* salt)
{
   int i;

   i = saltObs.indexOf(salt);

   if( i >= 0 ) {
      beginRemoveRows( QModelIndex(), i, i );
      disconnect( salt, nullptr, this, nullptr );
      saltObs.removeAt(i);
      endRemoveRows();

      if(parentTableWidget) {
         parentTableWidget->resizeColumnsToContents();
         parentTableWidget->resizeRowsToContents();
      }
   }
}

void SaltTableModel::removeSalts(QList<int>deadSalts)
{
   QList<Salt*> dead;

   // I am removing the salts so the index of any salt
   // will change. I think this will work
   foreach(int i, deadSalts) {
      dead.append( saltObs.at(i));
   }

   foreach( Salt* zombie, dead) {
      int i = saltObs.indexOf(zombie);

      if ( i >= 0 ) {
         beginRemoveRows( QModelIndex(), i, i );
         disconnect( zombie, nullptr, this, nullptr );
         saltObs.removeAt(i);
         endRemoveRows();
      }
   }

}

void SaltTableModel::removeAll()
{
   beginRemoveRows( QModelIndex(), 0, saltObs.size()-1 );
   while( !saltObs.isEmpty() ) {
      disconnect( saltObs.takeLast(), nullptr, this, nullptr );
   }
   endRemoveRows();
}

void SaltTableModel::changed(QMetaProperty prop, QVariant /*val*/)
{
   int i;

   // Find the notifier in the list
   Salt* saltSender = qobject_cast<Salt*>(sender());
   if( saltSender ) {
      i = saltObs.indexOf(saltSender);
      if( i >= 0 )
         emit dataChanged( QAbstractItemModel::createIndex(i, 0),
                           QAbstractItemModel::createIndex(i, SALTNUMCOLS-1));
         emit headerDataChanged( Qt::Vertical, i, i);
      return;
   }

   // See if sender is our recipe.
   Recipe* recSender = qobject_cast<Recipe*>(sender());
   if( recSender && recSender == recObs )
   {
      if( QString(prop.name()) == "salts" ) {
         removeAll();
         addSalts( recObs->salts() );
      }
      if( rowCount() > 0 )
         emit headerDataChanged( Qt::Vertical, 0, rowCount()-1 );
   }
}

int SaltTableModel::rowCount(const QModelIndex& /*parent*/) const
{
   return saltObs.size();
}

int SaltTableModel::columnCount(const QModelIndex& /*parent*/) const
{
   return SALTNUMCOLS;
}

QVariant SaltTableModel::data( const QModelIndex& index, int role ) const
{
   Salt* row;
   int col = index.column();
   Unit::unitScale scale;
   Unit::unitDisplay unit;

   // Ensure the row is ok.
   if( index.row() >= static_cast<int>(saltObs.size()) ) {
      Brewtarget::logW(tr("Bad model index. row = %1").arg(index.row()));
      return QVariant();
   }
   else
      row = saltObs[index.row()];

   switch( index.column() ) {
      case SALTNAMECOL:
         if ( role == Qt::DisplayRole )
            return QVariant( saltNames.at(row->type()));
         else if ( role == Qt::UserRole )
            return QVariant( row->type());
         else
            return QVariant();
      case SALTAMOUNTCOL:
         if ( role != Qt::DisplayRole )
            return QVariant();

         unit = displayUnit(col);
         scale = displayScale(col);

         return QVariant( Brewtarget::displayAmount(row->amount(), Units::kilograms,3, unit, scale));
      case SALTADDTOCOL:
         if ( role == Qt::DisplayRole )
            return QVariant( addToName.at(row->addTo()));
         else if ( role == Qt::UserRole )
            return QVariant( row->addTo());
         else
            return QVariant();
      default :
         Brewtarget::logW(tr("Bad column: %1").arg(index.column()));
         return QVariant();
   }
}

QVariant SaltTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
   if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
      switch( section ) {
         case SALTNAMECOL:
            return QVariant(tr("Name"));
         case SALTAMOUNTCOL:
            return QVariant(tr("Amount"));
         case SALTADDTOCOL:
            return QVariant(tr("Added To"));
         default:
            Brewtarget::logW(tr("Bad column: %1").arg(section));
            return QVariant();
      }
   }
   else
      return QVariant();
}

Qt::ItemFlags SaltTableModel::flags(const QModelIndex& index ) const
{
   Q_UNUSED(index)
   return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
}

bool SaltTableModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
   Salt *row;
   bool retval = false;

   if( index.row() >= saltObs.size() || role != Qt::EditRole ) {
      return false;
   }

   row = saltObs[index.row()];

   Unit::unitDisplay dspUnit = displayUnit(index.column());
   Unit::unitScale   dspScl  = displayScale(index.column());

   switch( index.column() ) {
      case SALTNAMECOL:
         retval = value.canConvert(QVariant::Int);
         if ( retval ) {
            row->setType( static_cast<Salt::Types>(value.toInt()));
            row->setName( saltNames.at(value.toInt()));
         }
         break;
      case SALTAMOUNTCOL:
         retval = value.canConvert(QVariant::Double);
         if ( retval ) {
            row->setAmount( Brewtarget::qStringToSI(value.toString(), Units::kilograms, dspUnit, dspScl) );
         }
         break;
      case SALTADDTOCOL:
         retval = value.canConvert(QVariant::Int);
         if ( retval ) {
            row->setAddTo( static_cast<Salt::WhenToAdd>(value.toInt()) );
         }
         break;
      default:
         retval = false;
         Brewtarget::logW(tr("Bad column: %1").arg(index.column()));
   }

   if ( retval )
      emit newTotals();
   emit dataChanged(index,index);
   return retval;
}

Unit::unitDisplay SaltTableModel::displayUnit(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noUnit;

   return static_cast<Unit::unitDisplay>(Brewtarget::option(attribute, QVariant(-1), this->objectName(), Brewtarget::UNIT).toInt());
}

Unit::unitScale SaltTableModel::displayScale(int column) const
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return Unit::noScale;

   return static_cast<Unit::unitScale>(Brewtarget::option(attribute, QVariant(-1), this->objectName(), Brewtarget::SCALE).toInt());
}

void SaltTableModel::setDisplayUnit(int column, Unit::unitDisplay displayUnit)
{
   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   Brewtarget::setOption(attribute,displayUnit,this->objectName(),Brewtarget::UNIT);
   Brewtarget::setOption(attribute,Unit::noScale,this->objectName(),Brewtarget::SCALE);

}

// Setting the scale should clear any cell-level scaling options
void SaltTableModel::setDisplayScale(int column, Unit::unitScale displayScale)
{

   QString attribute = generateName(column);

   if ( attribute.isEmpty() )
      return;

   Brewtarget::setOption(attribute,displayScale,this->objectName(),Brewtarget::SCALE);

}

QString SaltTableModel::generateName(int column) const
{
   QString attribute;

   switch(column)
   {
      case SALTAMOUNTCOL:
         attribute = "amount";
         break;
      default:
         attribute = "";
   }
   return attribute;
}

void SaltTableModel::contextMenu(const QPoint &point)
{
   QObject* calledBy = sender();
   QHeaderView* hView = qobject_cast<QHeaderView*>(calledBy);

   int selected = hView->logicalIndexAt(point);
   Unit::unitDisplay currentUnit;
   Unit::unitScale  currentScale;

   // Since we need to call generateVolumeMenu() two different ways, we need
   // to figure out the currentUnit and Scale here
   currentUnit  = displayUnit(selected);
   currentScale = displayScale(selected);

   QMenu* menu;
   QAction* invoked;

   switch(selected)
   {
      case SALTAMOUNTCOL:
         menu = Brewtarget::setupMassMenu(parentTableWidget,currentUnit, currentScale);
         break;
      default:
         return;
   }

   invoked = menu->exec(hView->mapToGlobal(point));
   if ( invoked == nullptr )
      return;

   QWidget* pMenu = invoked->parentWidget();
   if ( pMenu == menu )
      setDisplayUnit(selected,static_cast<Unit::unitDisplay>(invoked->data().toInt()));
   else
      setDisplayScale(selected,static_cast<Unit::unitScale>(invoked->data().toInt()));

}

//==========================CLASS SaltItemDelegate===============================

SaltItemDelegate::SaltItemDelegate(QObject* parent)
        : QItemDelegate(parent)
{
}

QWidget* SaltItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
   Q_UNUSED(option)
   if ( index.column() == SALTNAMECOL ) {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("NONE")  ,   Salt::NONE);
      box->addItem(tr("CaCL2") ,  Salt::CACL2);
      box->addItem(tr("CaCO3") ,  Salt::CACO3);
      box->addItem(tr("CaSO4") ,  Salt::CASO4);
      box->addItem(tr("MgSO4") ,  Salt::MGSO4);
      box->addItem(tr("NaCl")  ,   Salt::NACL);
      box->addItem(tr("NaHCO3"), Salt::NAHCO3);
      box->setMinimumWidth( box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;

   }
   else if ( index.column() == SALTADDTOCOL ) {
      QComboBox *box = new QComboBox(parent);

      box->addItem(tr("Never"),  Salt::NEVER);
      box->addItem(tr("Mash"),   Salt::MASH);
      box->addItem(tr("Sparge"), Salt::SPARGE);
      box->addItem(tr("Ratio"),  Salt::RATIO);
      box->addItem(tr("Equal"),  Salt::EQUAL);
      box->setMinimumWidth( box->minimumSizeHint().width());
      box->setSizeAdjustPolicy(QComboBox::AdjustToContents);
      return box;
   }
   else {
      return new QLineEdit(parent);
   }
}

void SaltItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   int column = index.column();

   if ( column == SALTNAMECOL || column == SALTADDTOCOL ) {
      QComboBox *box = qobject_cast<QComboBox*>(editor);
      box->setCurrentIndex(index.model()->data(index,Qt::UserRole).toInt());
   }
   else {
      QLineEdit* line = qobject_cast<QLineEdit*>(editor);
      line->setText(index.model()->data(index, Qt::DisplayRole).toString());
   }
}

void SaltItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
   int column = index.column();

   if ( column == SALTNAMECOL || column == SALTADDTOCOL ) {
      QComboBox* box = static_cast<QComboBox*>(editor);
      int selected = box->currentData().toInt();
      int saved = model->data(index,Qt::UserRole).toInt();

      if ( selected != saved ) {
         model->setData(index,selected,Qt::EditRole);
      }
   }
   else {
      QLineEdit* line = static_cast<QLineEdit*>(editor);

      if ( line->isModified() )
         model->setData(index, line->text(), Qt::EditRole);
   }
}

void SaltItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex& /*index*/) const
{
   editor->setGeometry(option.rect);
}
