#include "testing/testing.hpp"                                                                                                                                                  
                                                                                                                                                                                  
#include "qt/update_dialog.hpp"

#include <QtWidgets/QApplication>                                                                                                                                               
#include <QtWidgets/QTreeWidget>
                                                                                                                                                                                
namespace                   
{
// TODO: Would be nice to share this part between similar tests.
void EnsureApp()
{
  if (!QApplication::instance())
  {                                                                                                                                                                             
    qputenv("QT_QPA_PLATFORM", "offscreen");  // for headless CI
    static int argc = 0;                                                                                                                                                        
    static QApplication app(argc, nullptr);
  }                                                                                                                                                                             
}
                                                                                                                                                                                
void CheckChildrenSortedAlphabetically(QTreeWidgetItem const * parent)
{

  // Explicit null checks so the test fails clearly if any tree item is missing.
  TEST(parent != nullptr, ());

  for (int i = 1; i < parent->childCount(); ++i)
  {
    auto const * previous = parent->child(i - 1);
    auto const * current = parent->child(i);

    TEST(previous != nullptr, ());
    TEST(current != nullptr, ());

    // Direct comparison between country names. 
    TEST(QString::localeAwareCompare(previous->text(0), current->text(0)) <= 0,
         ("Expected alphabetical order, but got",
          previous->text(0).toStdString(),
          "before",
          current->text(0).toStdString()));
  }
}                                                                                                                                                           
                            
void CheckSubtreeSortedAlphabetically(QTreeWidgetItem const * parent)                                                                                                           
{
  // Explicit null checks so the test fails clearly if any tree item is missing.
  TEST(parent != nullptr, ());
  
  CheckChildrenSortedAlphabetically(parent);                                                                                                                                    
  for (int i = 0; i < parent->childCount(); ++i)
    CheckSubtreeSortedAlphabetically(parent->child(i));
}                                                                                                                                                                               
}  // namespace
                                                                                                                                                                                
UNIT_TEST(UpdateDialog_EntireTreeIsSortedAlphabetically)                                                                                                                        
{
  EnsureApp();                                                                                                                                                                  
                            
  Framework framework(FrameworkParams{}, false);
  qt::UpdateDialog dialog(nullptr, framework);
  dialog.FillTreeForTesting();                                                                                                                                                  

  QTreeWidget const & tree = dialog.GetTreeForTesting();                                                                                                                        
  TEST_EQUAL(tree.topLevelItemCount(), 1, ());
                                                                                                                                                                                
  QTreeWidgetItem const * root = tree.topLevelItem(0);
  TEST(root != nullptr, ());                                                                                                                                                    
  CheckSubtreeSortedAlphabetically(root);
}
