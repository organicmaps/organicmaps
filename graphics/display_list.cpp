#include "display_list.hpp"

namespace graphics
{
  DisplayList::DisplayList(DisplayListRenderer * parent)
    : m_parent(parent),
      m_isDebugging(false)
  {}

  DisplayList::~DisplayList()
  {
    for (list<shared_ptr<DiscardStorageCmd> >::const_iterator it = m_discardStorageCmd.begin();
         it != m_discardStorageCmd.end();
         ++it)
      m_parent->processCommand(*it);

    m_discardStorageCmd.clear();

    for (list<shared_ptr<FreeStorageCmd> >::const_iterator it = m_freeStorageCmd.begin();
         it != m_freeStorageCmd.end();
         ++it)
      m_parent->processCommand(*it);

    m_freeStorageCmd.clear();

    for (list<shared_ptr<FreeTextureCmd> >::const_iterator it = m_freeTextureCmd.begin();
         it != m_freeTextureCmd.end();
         ++it)
      m_parent->processCommand(*it);

    m_freeTextureCmd.clear();

    m_commands.clear();
  }

  void DisplayList::applyBlitStates(shared_ptr<ApplyBlitStatesCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_commands.push_back(cmd);
  }

  void DisplayList::applyStates(shared_ptr<ApplyStatesCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_commands.push_back(cmd);
  }

  void DisplayList::drawGeometry(shared_ptr<DrawGeometryCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_commands.push_back(cmd);
  }

  void DisplayList::unlockStorage(shared_ptr<UnlockStorageCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_parent->processCommand(cmd);
  }

  void DisplayList::freeStorage(shared_ptr<FreeStorageCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_freeStorageCmd.push_back(cmd);
  }

  void DisplayList::freeTexture(shared_ptr<FreeTextureCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_freeTextureCmd.push_back(cmd);
  }

  void DisplayList::discardStorage(shared_ptr<DiscardStorageCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_discardStorageCmd.push_back(cmd);
  }

  void DisplayList::uploadStyles(shared_ptr<UploadDataCmd> const & cmd)
  {
    cmd->setIsDebugging(m_isDebugging);
    m_parent->processCommand(cmd);
  }

  void DisplayList::addCheckPoint()
  {
    static_cast<gl::Renderer*>(m_parent)->addCheckPoint();
  }

  void DisplayList::draw(DisplayListRenderer * r,
                         math::Matrix<double, 3, 3> const & m)
  {
    math::Matrix<float, 4, 4> mv;

    /// preparing ModelView matrix

    mv(0, 0) = m(0, 0); mv(0, 1) = m(1, 0); mv(0, 2) = 0; mv(0, 3) = m(2, 0);
    mv(1, 0) = m(0, 1); mv(1, 1) = m(1, 1); mv(1, 2) = 0; mv(1, 3) = m(2, 1);
    mv(2, 0) = 0;       mv(2, 1) = 0;       mv(2, 2) = 1; mv(2, 3) = 0;
    mv(3, 0) = m(0, 2); mv(3, 1) = m(1, 2); mv(3, 2) = 0; mv(3, 3) = m(2, 2);

    r->loadMatrix(EModelView, mv);

    /// drawing collected geometry

    if (m_isDebugging)
      LOG(LINFO, ("started DisplayList::draw"));

    for (list<shared_ptr<Command> >::const_iterator it = m_commands.begin();
         it != m_commands.end();
         ++it)
      (*it)->perform();

    if (m_isDebugging)
      LOG(LINFO, ("finished DisplayList::draw"));

    r->loadMatrix(EModelView, math::Identity<double, 4>());
  }
}
